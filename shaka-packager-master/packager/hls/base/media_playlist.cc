// Copyright 2016 Google Inc. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "packager/hls/base/media_playlist.h"

#include <inttypes.h>

#include <algorithm>
#include <cmath>
#include <memory>

#include "packager/base/logging.h"
#include "packager/base/strings/string_number_conversions.h"
#include "packager/base/strings/stringprintf.h"
#include "packager/file/file.h"
#include "packager/hls/base/tag.h"
#include "packager/media/base/language_utils.h"
#include "packager/version/version.h"

namespace shaka {
namespace hls {

namespace {
uint32_t GetTimeScale(const MediaInfo& media_info) {
  if (media_info.has_reference_time_scale())
    return media_info.reference_time_scale();

  if (media_info.has_video_info())
    return media_info.video_info().time_scale();

  if (media_info.has_audio_info())
    return media_info.audio_info().time_scale();
  return 0u;
}

void AppendExtXMap(const MediaInfo& media_info, std::string* out) {
  if (media_info.has_init_segment_name()) {
    Tag tag("#EXT-X-MAP", out);
    tag.AddQuotedString("URI", media_info.init_segment_name().data());
    out->append("\n");
  } else if (media_info.has_media_file_name() && media_info.has_init_range()) {
    // It only makes sense for single segment media to have EXT-X-MAP if
    // there is init_range.
    Tag tag("#EXT-X-MAP", out);
    tag.AddQuotedString("URI", media_info.media_file_name().data());

    if (media_info.has_init_range()) {
      const uint64_t begin = media_info.init_range().begin();
      const uint64_t end = media_info.init_range().end();
      const uint64_t length = end - begin + 1;

      tag.AddQuotedNumberPair("BYTERANGE", length, '@', begin);
    }

    out->append("\n");
  } else {
    // This media info does not need an ext-x-map tag.
  }
}

std::string CreatePlaylistHeader(
    const MediaInfo& media_info,
    uint32_t target_duration,
    HlsPlaylistType type,
    MediaPlaylist::MediaPlaylistStreamType stream_type,
    int media_sequence_number,
    int discontinuity_sequence_number) {
  const std::string version = GetPackagerVersion();
  std::string version_line;
  if (!version.empty()) {
    version_line =
        base::StringPrintf("## Generated with %s version %s\n",
                           GetPackagerProjectUrl().c_str(), version.c_str());
  }

  // 6 is required for EXT-X-MAP without EXT-X-I-FRAMES-ONLY.
  std::string header = base::StringPrintf(
      "#EXTM3U\n"
      "#EXT-X-VERSION:6\n"
      "%s"
      "#EXT-X-TARGETDURATION:%d\n",
      version_line.c_str(), target_duration);

  switch (type) {
    case HlsPlaylistType::kVod:
      header += "#EXT-X-PLAYLIST-TYPE:VOD\n";
      break;
    case HlsPlaylistType::kEvent:
      header += "#EXT-X-PLAYLIST-TYPE:EVENT\n";
      break;
    case HlsPlaylistType::kLive:
      if (media_sequence_number > 0) {
        base::StringAppendF(&header, "#EXT-X-MEDIA-SEQUENCE:%d\n",
                            media_sequence_number);
      }
      if (discontinuity_sequence_number > 0) {
        base::StringAppendF(&header, "#EXT-X-DISCONTINUITY-SEQUENCE:%d\n",
                            discontinuity_sequence_number);
      }
      break;
    default:
      NOTREACHED() << "Unexpected MediaPlaylistType " << static_cast<int>(type);
  }
  if (stream_type ==
      MediaPlaylist::MediaPlaylistStreamType::kVideoIFramesOnly) {
    base::StringAppendF(&header, "#EXT-X-I-FRAMES-ONLY\n");
  }

  // Put EXT-X-MAP at the end since the rest of the playlist is about the
  // segment and key info.
  AppendExtXMap(media_info, &header);

  return header;
}

class SegmentInfoEntry : public HlsEntry {
 public:
  // If |use_byte_range| true then this will append EXT-X-BYTERANGE
  // after EXTINF.
  // It uses |previous_segment_end_offset| to determine if it has to also
  // specify the start byte offset in the tag.
  // |duration| is duration in seconds.
  SegmentInfoEntry(const std::string& file_name,
                   double start_time,
                   double duration,
                   bool use_byte_range,
                   uint64_t start_byte_offset,
                   uint64_t segment_file_size,
                   uint64_t previous_segment_end_offset);

  std::string ToString() override;
  double start_time() const { return start_time_; }
  double duration() const { return duration_; }

 private:
  SegmentInfoEntry(const SegmentInfoEntry&) = delete;
  SegmentInfoEntry& operator=(const SegmentInfoEntry&) = delete;

  const std::string file_name_;
  const double start_time_;
  const double duration_;
  const bool use_byte_range_;
  const uint64_t start_byte_offset_;
  const uint64_t segment_file_size_;
  const uint64_t previous_segment_end_offset_;
};

SegmentInfoEntry::SegmentInfoEntry(const std::string& file_name,
                                   double start_time,
                                   double duration,
                                   bool use_byte_range,
                                   uint64_t start_byte_offset,
                                   uint64_t segment_file_size,
                                   uint64_t previous_segment_end_offset)
    : HlsEntry(HlsEntry::EntryType::kExtInf),
      file_name_(file_name),
      start_time_(start_time),
      duration_(duration),
      use_byte_range_(use_byte_range),
      start_byte_offset_(start_byte_offset),
      segment_file_size_(segment_file_size),
      previous_segment_end_offset_(previous_segment_end_offset) {}

std::string SegmentInfoEntry::ToString() {
  std::string result = base::StringPrintf("#EXTINF:%.3f,", duration_);

  if (use_byte_range_) {
    base::StringAppendF(&result, "\n#EXT-X-BYTERANGE:%" PRIu64,
                        segment_file_size_);
    if (previous_segment_end_offset_ + 1 != start_byte_offset_) {
      base::StringAppendF(&result, "@%" PRIu64, start_byte_offset_);
    }
  }

  base::StringAppendF(&result, "\n%s", file_name_.c_str());

  return result;
}

class EncryptionInfoEntry : public HlsEntry {
 public:
  EncryptionInfoEntry(MediaPlaylist::EncryptionMethod method,
                      const std::string& url,
                      const std::string& key_id,
                      const std::string& iv,
                      const std::string& key_format,
                      const std::string& key_format_versions);

  std::string ToString() override;

 private:
  EncryptionInfoEntry(const EncryptionInfoEntry&) = delete;
  EncryptionInfoEntry& operator=(const EncryptionInfoEntry&) = delete;

  const MediaPlaylist::EncryptionMethod method_;
  const std::string url_;
  const std::string key_id_;
  const std::string iv_;
  const std::string key_format_;
  const std::string key_format_versions_;
};

EncryptionInfoEntry::EncryptionInfoEntry(MediaPlaylist::EncryptionMethod method,
                                         const std::string& url,
                                         const std::string& key_id,
                                         const std::string& iv,
                                         const std::string& key_format,
                                         const std::string& key_format_versions)
    : HlsEntry(HlsEntry::EntryType::kExtKey),
      method_(method),
      url_(url),
      key_id_(key_id),
      iv_(iv),
      key_format_(key_format),
      key_format_versions_(key_format_versions) {}

std::string EncryptionInfoEntry::ToString() {
  std::string tag_string;
  Tag tag("#EXT-X-KEY", &tag_string);

  if (method_ == MediaPlaylist::EncryptionMethod::kSampleAes) {
    tag.AddString("METHOD", "SAMPLE-AES");
  } else if (method_ == MediaPlaylist::EncryptionMethod::kAes128) {
    tag.AddString("METHOD", "AES-128");
  } else if (method_ == MediaPlaylist::EncryptionMethod::kSampleAesCenc) {
    tag.AddString("METHOD", "SAMPLE-AES-CTR");
  } else {
    DCHECK(method_ == MediaPlaylist::EncryptionMethod::kNone);
    tag.AddString("METHOD", "NONE");
  }

  tag.AddQuotedString("URI", url_);

  if (!key_id_.empty()) {
    tag.AddString("KEYID", key_id_);
  }
  if (!iv_.empty()) {
    tag.AddString("IV", iv_);
  }
  if (!key_format_versions_.empty()) {
    tag.AddQuotedString("KEYFORMATVERSIONS", key_format_versions_);
  }
  if (!key_format_.empty()) {
    tag.AddQuotedString("KEYFORMAT", key_format_);
  }

  return tag_string;
}

class DiscontinuityEntry : public HlsEntry {
 public:
  DiscontinuityEntry();

  std::string ToString() override;

 private:
  DiscontinuityEntry(const DiscontinuityEntry&) = delete;
  DiscontinuityEntry& operator=(const DiscontinuityEntry&) = delete;
};

DiscontinuityEntry::DiscontinuityEntry()
    : HlsEntry(HlsEntry::EntryType::kExtDiscontinuity) {}

std::string DiscontinuityEntry::ToString() {
  return "#EXT-X-DISCONTINUITY";
}

class PlacementOpportunityEntry : public HlsEntry {
 public:
  PlacementOpportunityEntry();

  std::string ToString() override;

 private:
  PlacementOpportunityEntry(const PlacementOpportunityEntry&) = delete;
  PlacementOpportunityEntry& operator=(const PlacementOpportunityEntry&) =
      delete;
};

PlacementOpportunityEntry::PlacementOpportunityEntry()
    : HlsEntry(HlsEntry::EntryType::kExtPlacementOpportunity) {}

std::string PlacementOpportunityEntry::ToString() {
  return "#EXT-X-PLACEMENT-OPPORTUNITY";
}

double LatestSegmentStartTime(
    const std::list<std::unique_ptr<HlsEntry>>& entries) {
  DCHECK(!entries.empty());
  for (auto iter = entries.rbegin(); iter != entries.rend(); ++iter) {
    if (iter->get()->type() == HlsEntry::EntryType::kExtInf) {
      const SegmentInfoEntry* segment_info =
          reinterpret_cast<SegmentInfoEntry*>(iter->get());
      return segment_info->start_time();
    }
  }
  return 0.0;
}

}  // namespace

HlsEntry::HlsEntry(HlsEntry::EntryType type) : type_(type) {}
HlsEntry::~HlsEntry() {}

MediaPlaylist::MediaPlaylist(HlsPlaylistType playlist_type,
                             double time_shift_buffer_depth,
                             const std::string& file_name,
                             const std::string& name,
                             const std::string& group_id)
    : playlist_type_(playlist_type),
      time_shift_buffer_depth_(time_shift_buffer_depth),
      file_name_(file_name),
      name_(name),
      group_id_(group_id) {}

MediaPlaylist::~MediaPlaylist() {}

void MediaPlaylist::SetStreamTypeForTesting(
    MediaPlaylistStreamType stream_type) {
  stream_type_ = stream_type;
}

void MediaPlaylist::SetCodecForTesting(const std::string& codec) {
  codec_ = codec;
}

bool MediaPlaylist::SetMediaInfo(const MediaInfo& media_info) {
  const uint32_t time_scale = GetTimeScale(media_info);
  if (time_scale == 0) {
    LOG(ERROR) << "MediaInfo does not contain a valid timescale.";
    return false;
  }

  if (media_info.has_video_info()) {
    stream_type_ = MediaPlaylistStreamType::kVideo;
    codec_ = media_info.video_info().codec();
  } else if (media_info.has_audio_info()) {
    stream_type_ = MediaPlaylistStreamType::kAudio;
    codec_ = media_info.audio_info().codec();
  } else {
    stream_type_ = MediaPlaylistStreamType::kSubtitle;
    codec_ = media_info.text_info().format();
  }

  time_scale_ = time_scale;
  media_info_ = media_info;
  use_byte_range_ = !media_info_.has_segment_template();
  return true;
}

void MediaPlaylist::AddSegment(const std::string& file_name,
                               uint64_t start_time,
                               uint64_t duration,
                               uint64_t start_byte_offset,
                               uint64_t size) {
  if (stream_type_ == MediaPlaylistStreamType::kVideoIFramesOnly) {
    if (key_frames_.empty())
      return;
    // Skip the last entry as the duration of the key frames are defined by the
    // next key frame, which we don't know yet.
    for (auto iter = key_frames_.begin(); iter != std::prev(key_frames_.end());
         ++iter) {
      const std::string& segment_file_name =
          iter->segment_file_name.empty() ? file_name : iter->segment_file_name;
      AddSegmentInfoEntry(segment_file_name, iter->timestamp, iter->duration,
                          iter->start_byte_offset, iter->size);
    }

    key_frames_.erase(key_frames_.begin(), std::prev(key_frames_.end()));
    KeyFrameInfo& key_frame = key_frames_.front();
    key_frame.segment_file_name = file_name;
    key_frame.duration = start_time + duration - key_frame.timestamp;
    return;
  }
  return AddSegmentInfoEntry(file_name, start_time, duration, start_byte_offset,
                             size);
}

void MediaPlaylist::AddKeyFrame(uint64_t timestamp,
                                uint64_t start_byte_offset,
                                uint64_t size) {
  if (stream_type_ != MediaPlaylistStreamType::kVideoIFramesOnly) {
    if (stream_type_ != MediaPlaylistStreamType::kVideo) {
      LOG(WARNING)
          << "I-Frames Only playlist applies to video renditions only.";
      return;
    }
    stream_type_ = MediaPlaylistStreamType::kVideoIFramesOnly;
    use_byte_range_ = true;
  }
  if (!key_frames_.empty()) {
    key_frames_.back().duration = timestamp - key_frames_.back().timestamp;
  }
  key_frames_.push_back({timestamp, start_byte_offset, size});
}

void MediaPlaylist::AddEncryptionInfo(MediaPlaylist::EncryptionMethod method,
                                      const std::string& url,
                                      const std::string& key_id,
                                      const std::string& iv,
                                      const std::string& key_format,
                                      const std::string& key_format_versions) {
  if (!inserted_discontinuity_tag_) {
    // Insert discontinuity tag only for the first EXT-X-KEY, only if there
    // are non-encrypted media segments.
    if (!entries_.empty())
      entries_.emplace_back(new DiscontinuityEntry());
    inserted_discontinuity_tag_ = true;
  }
  entries_.emplace_back(new EncryptionInfoEntry(
      method, url, key_id, iv, key_format, key_format_versions));
}

void MediaPlaylist::AddPlacementOpportunity() {
  entries_.emplace_back(new PlacementOpportunityEntry());
}

bool MediaPlaylist::WriteToFile(const std::string& file_path) {
  if (!key_frames_.empty() && playlist_type_ == HlsPlaylistType::kVod) {
    // Flush remaining key frames. This assumes |WriteToFile| is only called
    // once at the end of the file in VOD.
    CHECK_EQ(key_frames_.size(), 1u);
    const KeyFrameInfo& key_frame = key_frames_.front();
    AddSegmentInfoEntry(key_frame.segment_file_name, key_frame.timestamp,
                        key_frame.duration, key_frame.start_byte_offset,
                        key_frame.size);
    key_frames_.clear();
  }

  if (!target_duration_set_) {
    SetTargetDuration(ceil(GetLongestSegmentDuration()));
  }

  std::string content = CreatePlaylistHeader(
      media_info_, target_duration_, playlist_type_, stream_type_,
      media_sequence_number_, discontinuity_sequence_number_);

  for (const auto& entry : entries_)
    base::StringAppendF(&content, "%s\n", entry->ToString().c_str());

  if (playlist_type_ == HlsPlaylistType::kVod) {
    content += "#EXT-X-ENDLIST\n";
  }

  if (!File::WriteFileAtomically(file_path.c_str(), content)) {
    LOG(ERROR) << "Failed to write playlist to: " << file_path;
    return false;
  }
  return true;
}

uint64_t MediaPlaylist::Bitrate() const {
  if (media_info_.has_bandwidth())
    return media_info_.bandwidth();
  return max_bitrate_;
}

double MediaPlaylist::GetLongestSegmentDuration() const {
  return longest_segment_duration_;
}

void MediaPlaylist::SetTargetDuration(uint32_t target_duration) {
  if (target_duration_set_) {
    if (target_duration_ == target_duration)
      return;
    VLOG(1) << "Updating target duration from " << target_duration << " to "
            << target_duration_;
  }
  target_duration_ = target_duration;
  target_duration_set_ = true;
}

// Duplicated from MpdUtils because:
// 1. MpdUtils header depends on libxml header, which is not in the deps here
// 2. GetLanguage depends on MediaInfo from packager/mpd/
// 3. Moving GetLanguage to LanguageUtils would create a a media => mpd dep.
// TODO: fix this dependency situation and factor this out to a common location
std::string MediaPlaylist::GetLanguage() const {
  std::string lang;
  if (media_info_.has_audio_info()) {
    lang = media_info_.audio_info().language();
  } else if (media_info_.has_text_info()) {
    lang = media_info_.text_info().language();
  }
  return LanguageToShortestForm(lang);
}

int MediaPlaylist::GetNumChannels() const {
  return media_info_.audio_info().num_channels();
}

bool MediaPlaylist::GetDisplayResolution(uint32_t* width,
                                         uint32_t* height) const {
  DCHECK(width);
  DCHECK(height);
  if (media_info_.has_video_info()) {
    const double pixel_aspect_ratio =
        media_info_.video_info().pixel_height() > 0
            ? static_cast<double>(media_info_.video_info().pixel_width()) /
                  media_info_.video_info().pixel_height()
            : 1.0;
    *width = static_cast<uint32_t>(media_info_.video_info().width() *
                                   pixel_aspect_ratio);
    *height = media_info_.video_info().height();
    return true;
  }
  return false;
}

void MediaPlaylist::AddSegmentInfoEntry(const std::string& segment_file_name,
                                        uint64_t start_time,
                                        uint64_t duration,
                                        uint64_t start_byte_offset,
                                        uint64_t size) {
  if (time_scale_ == 0) {
    LOG(WARNING) << "Timescale is not set and the duration for " << duration
                 << " cannot be calculated. The output will be wrong.";

    entries_.emplace_back(new SegmentInfoEntry(
        segment_file_name, 0.0, 0.0, use_byte_range_, start_byte_offset, size,
        previous_segment_end_offset_));
    return;
  }

  const double start_time_seconds =
      static_cast<double>(start_time) / time_scale_;
  const double segment_duration_seconds =
      static_cast<double>(duration) / time_scale_;
  if (segment_duration_seconds > longest_segment_duration_)
    longest_segment_duration_ = segment_duration_seconds;

  const int kBitsInByte = 8;
  const uint64_t bitrate = kBitsInByte * size / segment_duration_seconds;
  max_bitrate_ = std::max(max_bitrate_, bitrate);
  entries_.emplace_back(new SegmentInfoEntry(
      segment_file_name, start_time_seconds, segment_duration_seconds,
      use_byte_range_, start_byte_offset, size, previous_segment_end_offset_));
  previous_segment_end_offset_ = start_byte_offset + size - 1;
  SlideWindow();
}

void MediaPlaylist::SlideWindow() {
  DCHECK(!entries_.empty());
  if (time_shift_buffer_depth_ <= 0.0 ||
      playlist_type_ != HlsPlaylistType::kLive) {
    return;
  }
  DCHECK_GT(time_scale_, 0u);

  // The start time of the latest segment is considered the current_play_time,
  // and this should guarantee that the latest segment will stay in the list.
  const double current_play_time = LatestSegmentStartTime(entries_);
  if (current_play_time <= time_shift_buffer_depth_)
    return;

  const double timeshift_limit = current_play_time - time_shift_buffer_depth_;

  // Temporary list to hold the EXT-X-KEYs. For example, this allows us to
  // remove <3> without removing <1> and <2> below (<1> and <2> are moved to the
  // temporary list and added back later).
  //    #EXT-X-KEY   <1>
  //    #EXT-X-KEY   <2>
  //    #EXTINF      <3>
  //    #EXTINF      <4>
  std::list<std::unique_ptr<HlsEntry>> ext_x_keys;
  // Consecutive key entries are either fully removed or not removed at all.
  // Keep track of entry types so we know if it is consecutive key entries.
  HlsEntry::EntryType prev_entry_type = HlsEntry::EntryType::kExtInf;

  std::list<std::unique_ptr<HlsEntry>>::iterator last = entries_.begin();
  size_t num_segments_removed = 0;
  for (; last != entries_.end(); ++last) {
    HlsEntry::EntryType entry_type = last->get()->type();
    if (entry_type == HlsEntry::EntryType::kExtKey) {
      if (prev_entry_type != HlsEntry::EntryType::kExtKey)
        ext_x_keys.clear();
      ext_x_keys.push_back(std::move(*last));
    } else if (entry_type == HlsEntry::EntryType::kExtDiscontinuity) {
      ++discontinuity_sequence_number_;
    } else {
      DCHECK_EQ(entry_type, HlsEntry::EntryType::kExtInf);
      const SegmentInfoEntry* segment_info =
          reinterpret_cast<SegmentInfoEntry*>(last->get());
      const double last_segment_end_time =
          segment_info->start_time() + segment_info->duration();
      if (timeshift_limit < last_segment_end_time)
        break;
      ++num_segments_removed;
    }
    prev_entry_type = entry_type;
  }
  entries_.erase(entries_.begin(), last);
  // Add key entries back.
  entries_.insert(entries_.begin(), std::make_move_iterator(ext_x_keys.begin()),
                  std::make_move_iterator(ext_x_keys.end()));
  media_sequence_number_ += num_segments_removed;
}

}  // namespace hls
}  // namespace shaka
