// Copyright 2016 Google Inc. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "packager/base/files/file_path.h"
#include "packager/file/file.h"
#include "packager/hls/base/master_playlist.h"
#include "packager/hls/base/media_playlist.h"
#include "packager/hls/base/mock_media_playlist.h"
#include "packager/version/version.h"

namespace shaka {
namespace hls {

using base::FilePath;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SetArgPointee;
using ::testing::StrEq;

namespace {
const char kDefaultMasterPlaylistName[] = "playlist.m3u8";
const char kDefaultLanguage[] = "en";
const uint32_t kWidth = 800;
const uint32_t kHeight = 600;
const HlsPlaylistType kVodPlaylist = HlsPlaylistType::kVod;

std::unique_ptr<MockMediaPlaylist> CreateVideoPlaylist(
    const std::string& filename,
    const std::string& codec,
    uint64_t bitrate) {
  const char kNoName[] = "";
  const char kNoGroup[] = "";

  std::unique_ptr<MockMediaPlaylist> playlist(
      new MockMediaPlaylist(kVodPlaylist, filename, kNoName, kNoGroup));

  playlist->SetStreamTypeForTesting(
      MediaPlaylist::MediaPlaylistStreamType::kVideo);
  playlist->SetCodecForTesting(codec);

  EXPECT_CALL(*playlist, Bitrate())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(bitrate));
  EXPECT_CALL(*playlist, GetDisplayResolution(NotNull(), NotNull()))
      .WillRepeatedly(DoAll(SetArgPointee<0>(kWidth), SetArgPointee<1>(kHeight),
                            Return(true)));

  return playlist;
}

std::unique_ptr<MockMediaPlaylist> CreateIframePlaylist(
    const std::string& filename,
    const std::string& codec,
    uint64_t bitrate) {
  auto playlist = CreateVideoPlaylist(filename, codec, bitrate);
  playlist->SetStreamTypeForTesting(
      MediaPlaylist::MediaPlaylistStreamType::kVideoIFramesOnly);
  return playlist;
}

std::unique_ptr<MockMediaPlaylist> CreateAudioPlaylist(
    const std::string& filename,
    const std::string& name,
    const std::string& group,
    const std::string& codec,
    const std::string& language,
    uint64_t channels,
    uint64_t bitrate) {
  std::unique_ptr<MockMediaPlaylist> playlist(
      new MockMediaPlaylist(kVodPlaylist, filename, name, group));

  EXPECT_CALL(*playlist, GetLanguage()).WillRepeatedly(Return(language));
  EXPECT_CALL(*playlist, GetNumChannels()).WillRepeatedly(Return(channels));

  playlist->SetStreamTypeForTesting(
      MediaPlaylist::MediaPlaylistStreamType::kAudio);
  playlist->SetCodecForTesting(codec);

  EXPECT_CALL(*playlist, Bitrate())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(bitrate));
  EXPECT_CALL(*playlist, GetDisplayResolution(NotNull(), NotNull())).Times(0);

  return playlist;
}

std::unique_ptr<MockMediaPlaylist> CreateTextPlaylist(
    const std::string& filename,
    const std::string& name,
    const std::string& group,
    const std::string& language) {
  std::unique_ptr<MockMediaPlaylist> playlist(
      new MockMediaPlaylist(kVodPlaylist, filename, name, group));

  EXPECT_CALL(*playlist, GetLanguage()).WillRepeatedly(Return(language));
  playlist->SetStreamTypeForTesting(
      MediaPlaylist::MediaPlaylistStreamType::kSubtitle);

  return playlist;
}
}  // namespace

class MasterPlaylistTest : public ::testing::Test {
 protected:
  MasterPlaylistTest()
      : master_playlist_(kDefaultMasterPlaylistName, kDefaultLanguage),
        test_output_dir_("memory://test_dir"),
        master_playlist_path_(
            FilePath::FromUTF8Unsafe(test_output_dir_)
                .Append(FilePath::FromUTF8Unsafe(kDefaultMasterPlaylistName))
                .AsUTF8Unsafe()) {}

  void SetUp() override { SetPackagerVersionForTesting("test"); }

  MasterPlaylist master_playlist_;
  std::string test_output_dir_;
  std::string master_playlist_path_;
};

TEST_F(MasterPlaylistTest, WriteMasterPlaylistOneVideo) {
  const uint64_t kBitRate = 435889;

  std::unique_ptr<MockMediaPlaylist> mock_playlist =
      CreateVideoPlaylist("media1.m3u8", "avc1", kBitRate);

  const char kBaseUrl[] = "http://myplaylistdomain.com/";
  EXPECT_TRUE(master_playlist_.WriteMasterPlaylist(kBaseUrl, test_output_dir_,
                                                   {mock_playlist.get()}));

  std::string actual;
  ASSERT_TRUE(File::ReadFileToString(master_playlist_path_.c_str(), &actual));

  const std::string expected =
      "#EXTM3U\n"
      "## Generated with https://github.com/google/shaka-packager version "
      "test\n"
      "\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=435889,CODECS=\"avc1\",RESOLUTION=800x600\n"
      "http://myplaylistdomain.com/media1.m3u8\n";

  ASSERT_EQ(expected, actual);
}

TEST_F(MasterPlaylistTest, WriteMasterPlaylistOneIframePlaylist) {
  const uint64_t kBitRate = 435889;

  std::unique_ptr<MockMediaPlaylist> mock_playlist =
      CreateIframePlaylist("media1.m3u8", "avc1", kBitRate);

  const char kBaseUrl[] = "http://myplaylistdomain.com/";
  EXPECT_TRUE(master_playlist_.WriteMasterPlaylist(kBaseUrl, test_output_dir_,
                                                   {mock_playlist.get()}));

  std::string actual;
  ASSERT_TRUE(File::ReadFileToString(master_playlist_path_.c_str(), &actual));

  const std::string expected =
      "#EXTM3U\n"
      "## Generated with https://github.com/google/shaka-packager version "
      "test\n"
      "\n"
      "#EXT-X-I-FRAME-STREAM-INF:BANDWIDTH=435889,CODECS=\"avc1\",RESOLUTION="
      "800x600,URI=\"http://myplaylistdomain.com/media1.m3u8\"\n";

  ASSERT_EQ(expected, actual);
}

TEST_F(MasterPlaylistTest, WriteMasterPlaylistVideoAndAudio) {
  const uint64_t kVideo1BitRate = 300000;
  const uint64_t kVideo2BitRate = 700000;

  const uint64_t kAudio1BitRate = 50000;
  const uint64_t kAudio2BitRate = 60000;

  const uint64_t kAudio1Channels = 2;
  const uint64_t kAudio2Channels = 5;

  // First video, sd.m3u8.
  std::unique_ptr<MockMediaPlaylist> sd_video_playlist =
      CreateVideoPlaylist("sd.m3u8", "sdvideocodec", kVideo1BitRate);

  // Second video, hd.m3u8.
  std::unique_ptr<MockMediaPlaylist> hd_video_playlist =
      CreateVideoPlaylist("hd.m3u8", "hdvideocodec", kVideo2BitRate);

  // First audio, english.m3u8.
  std::unique_ptr<MockMediaPlaylist> english_playlist =
      CreateAudioPlaylist("eng.m3u8", "english", "audiogroup", "audiocodec",
                          "en", kAudio1Channels, kAudio1BitRate);

  // Second audio, spanish.m3u8.
  std::unique_ptr<MockMediaPlaylist> spanish_playlist =
      CreateAudioPlaylist("spa.m3u8", "espanol", "audiogroup", "audiocodec",
                          "es", kAudio2Channels, kAudio2BitRate);

  const char kBaseUrl[] = "http://playlists.org/";
  EXPECT_TRUE(master_playlist_.WriteMasterPlaylist(
      kBaseUrl, test_output_dir_,
      {sd_video_playlist.get(), hd_video_playlist.get(), english_playlist.get(),
       spanish_playlist.get()}));

  std::string actual;
  ASSERT_TRUE(File::ReadFileToString(master_playlist_path_.c_str(), &actual));

  const std::string expected =
      "#EXTM3U\n"
      "## Generated with https://github.com/google/shaka-packager version "
      "test\n"
      "\n"
      "#EXT-X-MEDIA:TYPE=AUDIO,URI=\"http://playlists.org/eng.m3u8\","
      "GROUP-ID=\"audiogroup\",LANGUAGE=\"en\",NAME=\"english\","
      "DEFAULT=YES,AUTOSELECT=YES,CHANNELS=\"2\"\n"
      "#EXT-X-MEDIA:TYPE=AUDIO,URI=\"http://playlists.org/spa.m3u8\","
      "GROUP-ID=\"audiogroup\",LANGUAGE=\"es\",NAME=\"espanol\","
      "AUTOSELECT=YES,CHANNELS=\"5\"\n"
      "\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=360000,CODECS=\"sdvideocodec,audiocodec\","
      "RESOLUTION=800x600,AUDIO=\"audiogroup\"\n"
      "http://playlists.org/sd.m3u8\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=760000,CODECS=\"hdvideocodec,audiocodec\","
      "RESOLUTION=800x600,AUDIO=\"audiogroup\"\n"
      "http://playlists.org/hd.m3u8\n";

  ASSERT_EQ(expected, actual);
}

TEST_F(MasterPlaylistTest, WriteMasterPlaylistMultipleAudioGroups) {
  const uint64_t kVideoBitRate = 300000;

  const uint64_t kAudio1BitRate = 50000;
  const uint64_t kAudio2BitRate = 100000;

  const uint64_t kAudio1Channels = 1;
  const uint64_t kAudio2Channels = 8;

  // First video, sd.m3u8.
  std::unique_ptr<MockMediaPlaylist> video_playlist =
      CreateVideoPlaylist("video.m3u8", "videocodec", kVideoBitRate);

  // First audio, eng_lo.m3u8.
  std::unique_ptr<MockMediaPlaylist> eng_lo_playlist = CreateAudioPlaylist(
      "eng_lo.m3u8", "english_lo", "audio_lo", "audiocodec_lo", "en",
      kAudio1Channels, kAudio1BitRate);

  // Second audio, eng_hi.m3u8.
  std::unique_ptr<MockMediaPlaylist> eng_hi_playlist = CreateAudioPlaylist(
      "eng_hi.m3u8", "english_hi", "audio_hi", "audiocodec_hi", "en",
      kAudio2Channels, kAudio2BitRate);

  const char kBaseUrl[] = "http://anydomain.com/";
  EXPECT_TRUE(master_playlist_.WriteMasterPlaylist(
      kBaseUrl, test_output_dir_,
      {video_playlist.get(), eng_lo_playlist.get(), eng_hi_playlist.get()}));

  std::string actual;
  ASSERT_TRUE(File::ReadFileToString(master_playlist_path_.c_str(), &actual));

  const std::string expected =
      "#EXTM3U\n"
      "## Generated with https://github.com/google/shaka-packager version "
      "test\n"
      "\n"
      "#EXT-X-MEDIA:TYPE=AUDIO,URI=\"http://anydomain.com/eng_hi.m3u8\","
      "GROUP-ID=\"audio_hi\",LANGUAGE=\"en\",NAME=\"english_hi\","
      "DEFAULT=YES,AUTOSELECT=YES,CHANNELS=\"8\"\n"
      "#EXT-X-MEDIA:TYPE=AUDIO,URI=\"http://anydomain.com/eng_lo.m3u8\","
      "GROUP-ID=\"audio_lo\",LANGUAGE=\"en\",NAME=\"english_lo\","
      "DEFAULT=YES,AUTOSELECT=YES,CHANNELS=\"1\"\n"
      "\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=400000,CODECS=\"videocodec,audiocodec_hi\","
      "RESOLUTION=800x600,AUDIO=\"audio_hi\"\n"
      "http://anydomain.com/video.m3u8\n"
      "\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=350000,CODECS=\"videocodec,audiocodec_lo\","
      "RESOLUTION=800x600,AUDIO=\"audio_lo\"\n"
      "http://anydomain.com/video.m3u8\n";

  ASSERT_EQ(expected, actual);
}

TEST_F(MasterPlaylistTest, WriteMasterPlaylistSameAudioGroupSameLanguage) {
  // First video, video.m3u8.
  std::unique_ptr<MockMediaPlaylist> video_playlist =
      CreateVideoPlaylist("video.m3u8", "videocodec", 300000);

  // First audio, eng_lo.m3u8.
  std::unique_ptr<MockMediaPlaylist> eng_lo_playlist = CreateAudioPlaylist(
      "eng_lo.m3u8", "english", "audio", "audiocodec", "en", 1, 50000);

  std::unique_ptr<MockMediaPlaylist> eng_hi_playlist = CreateAudioPlaylist(
      "eng_hi.m3u8", "english", "audio", "audiocodec", "en", 8, 100000);

  const char kBaseUrl[] = "http://anydomain.com/";
  EXPECT_TRUE(master_playlist_.WriteMasterPlaylist(
      kBaseUrl, test_output_dir_,
      {video_playlist.get(), eng_lo_playlist.get(), eng_hi_playlist.get()}));

  std::string actual;
  ASSERT_TRUE(File::ReadFileToString(master_playlist_path_.c_str(), &actual));

  const std::string expected =
      "#EXTM3U\n"
      "## Generated with https://github.com/google/shaka-packager version "
      "test\n"
      "\n"
      "#EXT-X-MEDIA:TYPE=AUDIO,URI=\"http://anydomain.com/eng_lo.m3u8\","
      "GROUP-ID=\"audio\",LANGUAGE=\"en\",NAME=\"english\","
      "DEFAULT=YES,AUTOSELECT=YES,CHANNELS=\"1\"\n"
      "#EXT-X-MEDIA:TYPE=AUDIO,URI=\"http://anydomain.com/eng_hi.m3u8\","
      "GROUP-ID=\"audio\",LANGUAGE=\"en\",NAME=\"english\",CHANNELS=\"8\"\n"
      "\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=400000,CODECS=\"videocodec,audiocodec\","
      "RESOLUTION=800x600,AUDIO=\"audio\"\n"
      "http://anydomain.com/video.m3u8\n";

  ASSERT_EQ(expected, actual);
}

TEST_F(MasterPlaylistTest, WriteMasterPlaylistVideosAndTexts) {
  // Video, sd.m3u8.
  std::unique_ptr<MockMediaPlaylist> video1 =
      CreateVideoPlaylist("sd.m3u8", "sdvideocodec", 300000);

  // Video, hd.m3u8.
  std::unique_ptr<MockMediaPlaylist> video2 =
      CreateVideoPlaylist("hd.m3u8", "sdvideocodec", 600000);

  // Text, eng.m3u8.
  std::unique_ptr<MockMediaPlaylist> text_eng =
      CreateTextPlaylist("eng.m3u8", "english", "textgroup", "en");

  // Text, fr.m3u8.
  std::unique_ptr<MockMediaPlaylist> text_fr =
      CreateTextPlaylist("fr.m3u8", "french", "textgroup", "fr");

  const char kBaseUrl[] = "http://playlists.org/";
  EXPECT_TRUE(master_playlist_.WriteMasterPlaylist(
      kBaseUrl, test_output_dir_,
      {video1.get(), video2.get(), text_eng.get(), text_fr.get()}));

  std::string actual;
  ASSERT_TRUE(File::ReadFileToString(master_playlist_path_.c_str(), &actual));

  const std::string expected =
      "#EXTM3U\n"
      "## Generated with https://github.com/google/shaka-packager version "
      "test\n"
      "\n"
      "#EXT-X-MEDIA:TYPE=SUBTITLES,URI=\"http://playlists.org/eng.m3u8\","
      "GROUP-ID=\"textgroup\",LANGUAGE=\"en\",NAME=\"english\",DEFAULT=YES,"
      "AUTOSELECT=YES\n"
      "#EXT-X-MEDIA:TYPE=SUBTITLES,URI=\"http://playlists.org/fr.m3u8\","
      "GROUP-ID=\"textgroup\",LANGUAGE=\"fr\",NAME=\"french\",AUTOSELECT=YES\n"
      "\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=300000,CODECS=\"sdvideocodec\","
      "RESOLUTION=800x600,SUBTITLES=\"textgroup\"\n"
      "http://playlists.org/sd.m3u8\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=600000,CODECS=\"sdvideocodec\","
      "RESOLUTION=800x600,SUBTITLES=\"textgroup\"\n"
      "http://playlists.org/hd.m3u8\n";

  ASSERT_EQ(expected, actual);
}

TEST_F(MasterPlaylistTest, WriteMasterPlaylistVideoAndTextGroups) {
  // Video, sd.m3u8.
  std::unique_ptr<MockMediaPlaylist> video =
      CreateVideoPlaylist("sd.m3u8", "sdvideocodec", 300000);

  // Text, eng.m3u8.
  std::unique_ptr<MockMediaPlaylist> text_eng =
      CreateTextPlaylist("eng.m3u8", "english", "en-text-group", "en");

  // Text, fr.m3u8.
  std::unique_ptr<MockMediaPlaylist> text_fr =
      CreateTextPlaylist("fr.m3u8", "french", "fr-text-group", "fr");

  const char kBaseUrl[] = "http://playlists.org/";
  EXPECT_TRUE(master_playlist_.WriteMasterPlaylist(
      kBaseUrl, test_output_dir_,
      {video.get(), text_eng.get(), text_fr.get()}));

  std::string actual;
  ASSERT_TRUE(File::ReadFileToString(master_playlist_path_.c_str(), &actual));

  const std::string expected =
      "#EXTM3U\n"
      "## Generated with https://github.com/google/shaka-packager version "
      "test\n"
      "\n"
      "#EXT-X-MEDIA:TYPE=SUBTITLES,URI=\"http://playlists.org/eng.m3u8\","
      "GROUP-ID=\"en-text-group\",LANGUAGE=\"en\",NAME=\"english\","
      "DEFAULT=YES,AUTOSELECT=YES\n"
      "#EXT-X-MEDIA:TYPE=SUBTITLES,URI=\"http://playlists.org/fr.m3u8\","
      "GROUP-ID=\"fr-text-group\",LANGUAGE=\"fr\",NAME=\"french\","
      "AUTOSELECT=YES\n"
      "\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=300000,CODECS=\"sdvideocodec\","
      "RESOLUTION=800x600,SUBTITLES=\"en-text-group\"\n"
      "http://playlists.org/sd.m3u8\n"
      "\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=300000,CODECS=\"sdvideocodec\","
      "RESOLUTION=800x600,SUBTITLES=\"fr-text-group\"\n"
      "http://playlists.org/sd.m3u8\n";

  ASSERT_EQ(expected, actual);
}

TEST_F(MasterPlaylistTest, WriteMasterPlaylistVideoAndAudioAndText) {
  // Video, sd.m3u8.
  std::unique_ptr<MockMediaPlaylist> video =
      CreateVideoPlaylist("sd.m3u8", "sdvideocodec", 300000);

  // Audio, english.m3u8.
  std::unique_ptr<MockMediaPlaylist> audio = CreateAudioPlaylist(
      "eng.m3u8", "english", "audiogroup", "audiocodec", "en", 2, 50000);

  // Text, english.m3u8.
  std::unique_ptr<MockMediaPlaylist> text =
      CreateTextPlaylist("eng.m3u8", "english", "textgroup", "en");

  const char kBaseUrl[] = "http://playlists.org/";
  EXPECT_TRUE(master_playlist_.WriteMasterPlaylist(
      kBaseUrl, test_output_dir_, {video.get(), audio.get(), text.get()}));

  std::string actual;
  ASSERT_TRUE(File::ReadFileToString(master_playlist_path_.c_str(), &actual));

  const std::string expected =
      "#EXTM3U\n"
      "## Generated with https://github.com/google/shaka-packager version "
      "test\n"
      "\n"
      "#EXT-X-MEDIA:TYPE=AUDIO,URI=\"http://playlists.org/eng.m3u8\","
      "GROUP-ID=\"audiogroup\",LANGUAGE=\"en\",NAME=\"english\","
      "DEFAULT=YES,AUTOSELECT=YES,CHANNELS=\"2\"\n"
      "\n"
      "#EXT-X-MEDIA:TYPE=SUBTITLES,URI=\"http://playlists.org/eng.m3u8\","
      "GROUP-ID=\"textgroup\",LANGUAGE=\"en\",NAME=\"english\",DEFAULT=YES,"
      "AUTOSELECT=YES\n"
      "\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=350000,CODECS=\"sdvideocodec,audiocodec\","
      "RESOLUTION=800x600,AUDIO=\"audiogroup\",SUBTITLES=\"textgroup\"\n"
      "http://playlists.org/sd.m3u8\n";

  ASSERT_EQ(expected, actual);
}

TEST_F(MasterPlaylistTest, WriteMasterPlaylistMixedPlaylistsDifferentGroups) {
  const uint64_t kAudioChannels = 2;
  const uint64_t kAudioBitRate = 50000;
  const uint64_t kVideoBitRate = 300000;
  const uint64_t kIframeBitRate = 100000;

  std::unique_ptr<MockMediaPlaylist> media_playlists[] = {
      // AUDIO
      CreateAudioPlaylist("audio-1.m3u8", "audio 1", "audio-group-1",
                          "audiocodec", "en", kAudioChannels, kAudioBitRate),
      CreateAudioPlaylist("audio-2.m3u8", "audio 2", "audio-group-2",
                          "audiocodec", "en", kAudioChannels, kAudioBitRate),

      // SUBTITLES
      CreateTextPlaylist("text-1.m3u8", "text 1", "text-group-1", "en"),
      CreateTextPlaylist("text-2.m3u8", "text 2", "text-group-2", "en"),

      // VIDEO
      CreateVideoPlaylist("video-1.m3u8", "sdvideocodec", kVideoBitRate),
      CreateVideoPlaylist("video-2.m3u8", "sdvideocodec", kVideoBitRate),

      // I-Frame
      CreateIframePlaylist("iframe-1.m3u8", "sdvideocodec", kIframeBitRate),
      CreateIframePlaylist("iframe-2.m3u8", "sdvideocodec", kIframeBitRate),
  };

  // Add all the media playlists to the master playlist.
  std::list<MediaPlaylist*> media_playlist_list;
  for (const auto& media_playlist : media_playlists) {
    media_playlist_list.push_back(media_playlist.get());
  }

  const char kBaseUrl[] = "http://playlists.org/";
  EXPECT_TRUE(master_playlist_.WriteMasterPlaylist(kBaseUrl, test_output_dir_,
                                                   media_playlist_list));

  std::string actual;
  ASSERT_TRUE(File::ReadFileToString(master_playlist_path_.c_str(), &actual));

  const std::string expected =
      "#EXTM3U\n"
      "## Generated with https://github.com/google/shaka-packager version "
      "test\n"
      "\n"
      "#EXT-X-MEDIA:TYPE=AUDIO,URI=\"http://playlists.org/audio-1.m3u8\","
      "GROUP-ID=\"audio-group-1\",LANGUAGE=\"en\",NAME=\"audio 1\","
      "DEFAULT=YES,AUTOSELECT=YES,CHANNELS=\"2\"\n"
      "#EXT-X-MEDIA:TYPE=AUDIO,URI=\"http://playlists.org/audio-2.m3u8\","
      "GROUP-ID=\"audio-group-2\",LANGUAGE=\"en\",NAME=\"audio 2\","
      "DEFAULT=YES,AUTOSELECT=YES,CHANNELS=\"2\"\n"
      "\n"
      "#EXT-X-MEDIA:TYPE=SUBTITLES,URI=\"http://playlists.org/text-1.m3u8\","
      "GROUP-ID=\"text-group-1\",LANGUAGE=\"en\",NAME=\"text 1\","
      "DEFAULT=YES,AUTOSELECT=YES\n"
      "#EXT-X-MEDIA:TYPE=SUBTITLES,URI=\"http://playlists.org/text-2.m3u8\","
      "GROUP-ID=\"text-group-2\",LANGUAGE=\"en\",NAME=\"text 2\","
      "DEFAULT=YES,AUTOSELECT=YES\n"
      "\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=350000,CODECS=\"sdvideocodec,audiocodec\","
      "RESOLUTION=800x600,AUDIO=\"audio-group-1\",SUBTITLES=\"text-group-1\"\n"
      "http://playlists.org/video-1.m3u8\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=350000,CODECS=\"sdvideocodec,audiocodec\","
      "RESOLUTION=800x600,AUDIO=\"audio-group-1\",SUBTITLES=\"text-group-1\"\n"
      "http://playlists.org/video-2.m3u8\n"
      "\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=350000,CODECS=\"sdvideocodec,audiocodec\","
      "RESOLUTION=800x600,AUDIO=\"audio-group-1\",SUBTITLES=\"text-group-2\"\n"
      "http://playlists.org/video-1.m3u8\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=350000,CODECS=\"sdvideocodec,audiocodec\","
      "RESOLUTION=800x600,AUDIO=\"audio-group-1\",SUBTITLES=\"text-group-2\"\n"
      "http://playlists.org/video-2.m3u8\n"
      "\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=350000,CODECS=\"sdvideocodec,audiocodec\","
      "RESOLUTION=800x600,AUDIO=\"audio-group-2\",SUBTITLES=\"text-group-1\"\n"
      "http://playlists.org/video-1.m3u8\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=350000,CODECS=\"sdvideocodec,audiocodec\","
      "RESOLUTION=800x600,AUDIO=\"audio-group-2\",SUBTITLES=\"text-group-1\"\n"
      "http://playlists.org/video-2.m3u8\n"
      "\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=350000,CODECS=\"sdvideocodec,audiocodec\","
      "RESOLUTION=800x600,AUDIO=\"audio-group-2\",SUBTITLES=\"text-group-2\"\n"
      "http://playlists.org/video-1.m3u8\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=350000,CODECS=\"sdvideocodec,audiocodec\","
      "RESOLUTION=800x600,AUDIO=\"audio-group-2\",SUBTITLES=\"text-group-2\"\n"
      "http://playlists.org/video-2.m3u8\n"
      "\n"
      "#EXT-X-I-FRAME-STREAM-INF:BANDWIDTH=100000,CODECS=\"sdvideocodec\","
      "RESOLUTION=800x600,URI=\"http://playlists.org/iframe-1.m3u8\"\n"
      "#EXT-X-I-FRAME-STREAM-INF:BANDWIDTH=100000,CODECS=\"sdvideocodec\","
      "RESOLUTION=800x600,URI=\"http://playlists.org/iframe-2.m3u8\"\n";

  ASSERT_EQ(expected, actual);
}
}  // namespace hls
}  // namespace shaka
