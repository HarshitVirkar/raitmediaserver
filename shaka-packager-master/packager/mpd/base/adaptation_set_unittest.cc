// Copyright 2017 Google Inc. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "packager/mpd/base/adaptation_set.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "packager/mpd/base/content_protection_element.h"
#include "packager/mpd/base/mpd_options.h"
#include "packager/mpd/base/representation.h"
#include "packager/mpd/test/mpd_builder_test_helper.h"
#include "packager/mpd/test/xml_compare.h"

using ::testing::ElementsAre;
using ::testing::Not;

namespace shaka {

namespace {
const uint32_t kAnyAdaptationSetId = 1;
const char kNoLanguage[] = "";
}  // namespace

class AdaptationSetTest : public ::testing::Test {
 public:
  std::unique_ptr<AdaptationSet> CreateAdaptationSet(uint32_t adaptation_set_id,
                                                     const std::string& lang) {
    return std::unique_ptr<AdaptationSet>(new AdaptationSet(
        adaptation_set_id, lang, mpd_options_, &representation_counter_));
  }

 protected:
  MpdOptions mpd_options_;
  base::AtomicSequenceNumber representation_counter_;
};

class OnDemandAdaptationSetTest : public AdaptationSetTest {
 public:
  void SetUp() override { mpd_options_.dash_profile = DashProfile::kOnDemand; }
};

class LiveAdaptationSetTest : public AdaptationSetTest {
 public:
  void SetUp() override { mpd_options_.dash_profile = DashProfile::kLive; }
};

TEST_F(AdaptationSetTest, AddAdaptationSetSwitching) {
  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  adaptation_set->AddAdaptationSetSwitching(1);
  adaptation_set->AddAdaptationSetSwitching(2);
  adaptation_set->AddAdaptationSetSwitching(8);

  // The empty contentType is sort of a side effect of being able to generate an
  // MPD without adding any Representations.
  const char kExpectedOutput[] =
      "<AdaptationSet id=\"1\" contentType=\"\">"
      "  <SupplementalProperty "
      "   schemeIdUri=\"urn:mpeg:dash:adaptation-set-switching:2016\" "
      "   value=\"1,2,8\"/>"
      "</AdaptationSet>";
  EXPECT_THAT(adaptation_set->GetXml().get(), XmlNodeEqual(kExpectedOutput));
}

// Verify that content type is set correctly if video info is present in
// MediaInfo.
TEST_F(AdaptationSetTest, CheckAdaptationSetVideoContentType) {
  const char kVideoMediaInfo[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width: 1280\n"
      "  height: 720\n"
      "  time_scale: 10\n"
      "  frame_duration: 10\n"
      "  pixel_width: 1\n"
      "  pixel_height: 1\n"
      "}\n"
      "container_type: CONTAINER_MP4\n";

  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  adaptation_set->AddRepresentation(ConvertToMediaInfo(kVideoMediaInfo));
  EXPECT_THAT(adaptation_set->GetXml().get(),
              AttributeEqual("contentType", "video"));
}

// Verify that content type is set correctly if audio info is present in
// MediaInfo.
TEST_F(AdaptationSetTest, CheckAdaptationSetAudioContentType) {
  const char kAudioMediaInfo[] =
      "audio_info {\n"
      "  codec: 'mp4a.40.2'\n"
      "  sampling_frequency: 44100\n"
      "  time_scale: 1200\n"
      "  num_channels: 2\n"
      "}\n"
      "container_type: CONTAINER_MP4\n";

  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  adaptation_set->AddRepresentation(ConvertToMediaInfo(kAudioMediaInfo));
  EXPECT_THAT(adaptation_set->GetXml().get(),
              AttributeEqual("contentType", "audio"));
}

// Verify that content type is set correctly if text info is present in
// MediaInfo.
TEST_F(AdaptationSetTest, CheckAdaptationSetTextContentType) {
  const char kTextMediaInfo[] =
      "text_info {\n"
      "  format: 'ttml'\n"
      "  language: 'en'\n"
      "}\n"
      "container_type: CONTAINER_TEXT\n";

  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, "en");
  adaptation_set->AddRepresentation(ConvertToMediaInfo(kTextMediaInfo));
  EXPECT_THAT(adaptation_set->GetXml().get(),
              AttributeEqual("contentType", "text"));
}

TEST_F(AdaptationSetTest, CopyRepresentationWithTimeOffset) {
  const char kVideoMediaInfo[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width: 1280\n"
      "  height: 720\n"
      "  time_scale: 10\n"
      "  frame_duration: 10\n"
      "  pixel_width: 1\n"
      "  pixel_height: 1\n"
      "}\n"
      "container_type: CONTAINER_MP4\n";

  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  Representation* representation =
      adaptation_set->AddRepresentation(ConvertToMediaInfo(kVideoMediaInfo));

  const uint64_t kPresentationTimeOffset = 80;
  Representation* new_representation =
      adaptation_set->CopyRepresentationWithTimeOffset(*representation,
                                                       kPresentationTimeOffset);
  EXPECT_EQ(kPresentationTimeOffset,
            new_representation->GetMediaInfo().presentation_time_offset());
}

// Verify that language passed to the constructor sets the @lang field is set.
TEST_F(AdaptationSetTest, CheckLanguageAttributeSet) {
  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, "en");
  EXPECT_THAT(adaptation_set->GetXml().get(), AttributeEqual("lang", "en"));
}

// Verify that language tags with subtags can still be converted.
TEST_F(AdaptationSetTest, CheckConvertLanguageWithSubtag) {
  // "por-BR" is the long tag for Brazillian Portuguese.  The short tag
  // is "pt-BR", which is what should appear in the manifest.
  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, "por-BR");
  EXPECT_THAT(adaptation_set->GetXml().get(), AttributeEqual("lang", "pt-BR"));
}

TEST_F(AdaptationSetTest, CheckAdaptationSetId) {
  const uint32_t kAdaptationSetId = 42;
  auto adaptation_set = CreateAdaptationSet(kAdaptationSetId, kNoLanguage);
  EXPECT_THAT(adaptation_set->GetXml().get(),
              AttributeEqual("id", std::to_string(kAdaptationSetId)));
}

// Verify AdaptationSet::AddRole() works for "main" role.
TEST_F(AdaptationSetTest, AdaptationAddRoleElementMain) {
  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  adaptation_set->AddRole(AdaptationSet::kRoleMain);

  // The empty contentType is sort of a side effect of being able to generate an
  // MPD without adding any Representations.
  const char kExpectedOutput[] =
      "<AdaptationSet id=\"1\" contentType=\"\">\n"
      "  <Role schemeIdUri=\"urn:mpeg:dash:role:2011\" value=\"main\"/>\n"
      "</AdaptationSet>";
  EXPECT_THAT(adaptation_set->GetXml().get(), XmlNodeEqual(kExpectedOutput));
}

// Add Role, ContentProtection, and Representation elements. Verify that
// ContentProtection -> Role -> Representation are in order.
TEST_F(AdaptationSetTest, CheckContentProtectionRoleRepresentationOrder) {
  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  adaptation_set->AddRole(AdaptationSet::kRoleMain);
  ContentProtectionElement any_content_protection;
  any_content_protection.scheme_id_uri = "any_scheme";
  adaptation_set->AddContentProtectionElement(any_content_protection);
  const char kAudioMediaInfo[] =
      "audio_info {\n"
      "  codec: 'mp4a.40.2'\n"
      "  sampling_frequency: 44100\n"
      "  time_scale: 1200\n"
      "  num_channels: 2\n"
      "}\n"
      "container_type: 1\n";
  adaptation_set->AddRepresentation(ConvertToMediaInfo(kAudioMediaInfo));

  xml::scoped_xml_ptr<xmlNode> adaptation_set_xml(adaptation_set->GetXml());
  const char kExpectedOutput[] =
      "<AdaptationSet id=\"1\" contentType=\"audio\">\n"
      "  <ContentProtection schemeIdUri=\"any_scheme\"/>\n"
      "  <Role schemeIdUri=\"urn:mpeg:dash:role:2011\" value=\"main\"/>\n"
      "  <Representation id=\"0\" bandwidth=\"0\" codecs=\"mp4a.40.2\"\n"
      "   mimeType=\"audio/mp4\" audioSamplingRate=\"44100\">\n"
      "    <AudioChannelConfiguration\n"
      "     schemeIdUri=\n"
      "      \"urn:mpeg:dash:23003:3:audio_channel_configuration:2011\"\n"
      "     value=\"2\"/>\n"
      "  </Representation>\n"
      "</AdaptationSet>";
  EXPECT_THAT(adaptation_set->GetXml().get(), XmlNodeEqual(kExpectedOutput));
}

// Verify that if all video Representations in an AdaptationSet have the same
// frame rate, AdaptationSet also has a frameRate attribute.
TEST_F(AdaptationSetTest, AdapatationSetFrameRate) {
  const char kVideoMediaInfo1[] =
      "video_info {\n"
      "  codec: \"avc1\"\n"
      "  width: 720\n"
      "  height: 480\n"
      "  time_scale: 10\n"
      "  frame_duration: 3\n"
      "}\n"
      "container_type: 1\n";
  const char kVideoMediaInfo2[] =
      "video_info {\n"
      "  codec: \"avc1\"\n"
      "  width: 720\n"
      "  height: 480\n"
      "  time_scale: 10\n"
      "  frame_duration: 3\n"
      "}\n"
      "container_type: 1\n";
  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  ASSERT_TRUE(
      adaptation_set->AddRepresentation(ConvertToMediaInfo(kVideoMediaInfo1)));
  ASSERT_TRUE(
      adaptation_set->AddRepresentation(ConvertToMediaInfo(kVideoMediaInfo2)));

  xml::scoped_xml_ptr<xmlNode> adaptation_set_xml(adaptation_set->GetXml());
  EXPECT_THAT(adaptation_set_xml.get(), AttributeEqual("frameRate", "10/3"));
  EXPECT_THAT(adaptation_set_xml.get(), Not(AttributeSet("maxFrameRate")));
}

// Verify that if there are videos with different frame rates, the maxFrameRate
// is set.
TEST_F(AdaptationSetTest, AdapatationSetMaxFrameRate) {
  // 30fps video.
  const char kVideoMediaInfo30fps[] =
      "video_info {\n"
      "  codec: \"avc1\"\n"
      "  width: 720\n"
      "  height: 480\n"
      "  time_scale: 3000\n"
      "  frame_duration: 100\n"
      "}\n"
      "container_type: 1\n";
  const char kVideoMediaInfo15fps[] =
      "video_info {\n"
      "  codec: \"avc1\"\n"
      "  width: 720\n"
      "  height: 480\n"
      "  time_scale: 3000\n"
      "  frame_duration: 200\n"
      "}\n"
      "container_type: 1\n";
  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  ASSERT_TRUE(adaptation_set->AddRepresentation(
      ConvertToMediaInfo(kVideoMediaInfo30fps)));
  ASSERT_TRUE(adaptation_set->AddRepresentation(
      ConvertToMediaInfo(kVideoMediaInfo15fps)));

  xml::scoped_xml_ptr<xmlNode> adaptation_set_xml(adaptation_set->GetXml());
  EXPECT_THAT(adaptation_set_xml.get(),
              AttributeEqual("maxFrameRate", "3000/100"));
  EXPECT_THAT(adaptation_set_xml.get(), Not(AttributeSet("frameRate")));
}

// Verify that (max)FrameRate can be set by calling
// Representation::SetSampleDuration().
TEST_F(AdaptationSetTest,
       SetAdaptationFrameRateUsingRepresentationSetSampleDuration) {
  // Note that frame duration is not set in the MediaInfos. It could be there
  // and should not affect the behavior of the program.
  // But to make it closer to a real live-profile use case,
  // the frame duration is not set in the MediaInfo, instead it is set using
  // SetSampleDuration().
  const char k480pMediaInfo[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width: 720\n"
      "  height: 480\n"
      "  time_scale: 10\n"
      "  pixel_width: 8\n"
      "  pixel_height: 9\n"
      "}\n"
      "container_type: 1\n";
  const char k360pMediaInfo[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width: 640\n"
      "  height: 360\n"
      "  time_scale: 10\n"
      "  pixel_width: 1\n"
      "  pixel_height: 1\n"
      "}\n"
      "container_type: 1\n";

  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  Representation* representation_480p =
      adaptation_set->AddRepresentation(ConvertToMediaInfo(k480pMediaInfo));
  Representation* representation_360p =
      adaptation_set->AddRepresentation(ConvertToMediaInfo(k360pMediaInfo));

  // First, make sure that maxFrameRate nor frameRate are set because
  // frame durations were not provided in the MediaInfo.
  xml::scoped_xml_ptr<xmlNode> no_frame_rate(adaptation_set->GetXml());
  EXPECT_THAT(no_frame_rate.get(), Not(AttributeSet("maxFrameRate")));
  EXPECT_THAT(no_frame_rate.get(), Not(AttributeSet("frameRate")));

  // Then set same frame duration for the representations. (Given that the
  // time scales match).
  const uint32_t kSameFrameDuration = 3u;
  representation_480p->SetSampleDuration(kSameFrameDuration);
  representation_360p->SetSampleDuration(kSameFrameDuration);

  xml::scoped_xml_ptr<xmlNode> same_frame_rate(adaptation_set->GetXml());
  EXPECT_THAT(same_frame_rate.get(), Not(AttributeSet("maxFrameRate")));
  EXPECT_THAT(same_frame_rate.get(), AttributeEqual("frameRate", "10/3"));

  // Then set 480p to be 5fps (10/2) so that maxFrameRate is set.
  const uint32_t k5FPSFrameDuration = 2;
  static_assert(k5FPSFrameDuration < kSameFrameDuration,
                "frame_duration_must_be_shorter_for_max_frame_rate");
  representation_480p->SetSampleDuration(k5FPSFrameDuration);

  xml::scoped_xml_ptr<xmlNode> max_frame_rate(adaptation_set->GetXml());
  EXPECT_THAT(max_frame_rate.get(), AttributeEqual("maxFrameRate", "10/2"));
  EXPECT_THAT(max_frame_rate.get(), Not(AttributeSet("frameRate")));
}

// Verify that if the picture aspect ratio of all the Representations are the
// same, @par attribute is present.
TEST_F(AdaptationSetTest, AdaptationSetParAllSame) {
  const char k480pVideoInfo[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width:  854\n"
      "  height: 480\n"
      "  time_scale: 3000\n"
      "  frame_duration: 100\n"
      "  pixel_width: 1\n"
      "  pixel_height: 1\n"
      "}\n"
      "container_type: 1\n";
  const char k720pVideoInfo[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width: 1280\n"
      "  height: 720\n"
      "  time_scale: 3000\n"
      "  frame_duration: 100\n"
      "  pixel_width: 1\n"
      "  pixel_height: 1\n"
      "}\n"
      "container_type: 1\n";
  const char k1080pVideoInfo[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width: 1920\n"
      "  height: 1080\n"
      "  time_scale: 3000\n"
      "  frame_duration: 100\n"
      "  pixel_width: 1\n"
      "  pixel_height: 1\n"
      "}\n"
      "container_type: 1\n";

  // Note that this has non-1 pixel width and height.
  // Which makes the par 16:9.
  const char k360pVideoInfo[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width: 720\n"
      "  height: 360\n"
      "  time_scale: 3000\n"
      "  frame_duration: 100\n"
      "  pixel_width: 8\n"
      "  pixel_height: 9\n"
      "}\n"
      "container_type: 1\n";

  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  ASSERT_TRUE(
      adaptation_set->AddRepresentation(ConvertToMediaInfo(k480pVideoInfo)));
  ASSERT_TRUE(
      adaptation_set->AddRepresentation(ConvertToMediaInfo(k720pVideoInfo)));
  ASSERT_TRUE(
      adaptation_set->AddRepresentation(ConvertToMediaInfo(k1080pVideoInfo)));
  ASSERT_TRUE(
      adaptation_set->AddRepresentation(ConvertToMediaInfo(k360pVideoInfo)));

  xml::scoped_xml_ptr<xmlNode> adaptation_set_xml(adaptation_set->GetXml());
  EXPECT_THAT(adaptation_set_xml.get(), AttributeEqual("par", "16:9"));
}

// Verify that adding Representations with different par will generate
// AdaptationSet without @par.
TEST_F(AdaptationSetTest, AdaptationSetParDifferent) {
  const char k16by9VideoInfo[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width: 1280\n"
      "  height: 720\n"
      "  time_scale: 3000\n"
      "  frame_duration: 100\n"
      "  pixel_width: 1\n"
      "  pixel_height: 1\n"
      "}\n"
      "container_type: 1\n";
  // Note that 720:360 is 2:1 where as 720p (above) is 16:9.
  const char k2by1VideoInfo[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width: 720\n"
      "  height: 360\n"
      "  time_scale: 3000\n"
      "  frame_duration: 100\n"
      "  pixel_width: 1\n"
      "  pixel_height: 1\n"
      "}\n"
      "container_type: 1\n";

  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  ASSERT_TRUE(
      adaptation_set->AddRepresentation(ConvertToMediaInfo(k16by9VideoInfo)));
  ASSERT_TRUE(
      adaptation_set->AddRepresentation(ConvertToMediaInfo(k2by1VideoInfo)));

  xml::scoped_xml_ptr<xmlNode> adaptation_set_xml(adaptation_set->GetXml());
  EXPECT_THAT(adaptation_set_xml.get(), Not(AttributeSet("par")));
}

// Verify that adding Representation without pixel_width and pixel_height will
// not set @par.
TEST_F(AdaptationSetTest, AdaptationSetParUnknown) {
  const char kUknownPixelWidthAndHeight[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width: 1280\n"
      "  height: 720\n"
      "  time_scale: 3000\n"
      "  frame_duration: 100\n"
      "}\n"
      "container_type: 1\n";

  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  ASSERT_TRUE(adaptation_set->AddRepresentation(
      ConvertToMediaInfo(kUknownPixelWidthAndHeight)));

  xml::scoped_xml_ptr<xmlNode> adaptation_set_xml(adaptation_set->GetXml());
  EXPECT_THAT(adaptation_set_xml.get(), Not(AttributeSet("par")));
}

// Catch the case where it ends up wrong if integer division is used to check
// the frame rate.
// IOW, A/B != C/D but when using integer division A/B == C/D.
// SO, maxFrameRate should be set instead of frameRate.
TEST_F(AdaptationSetTest, AdapatationSetMaxFrameRateIntegerDivisionEdgeCase) {
  // 11/3 != 10/3 but IntegerDiv(11,3) == IntegerDiv(10,3).
  const char kVideoMediaInfo1[] =
      "video_info {\n"
      "  codec: \"avc1\"\n"
      "  width: 720\n"
      "  height: 480\n"
      "  time_scale: 11\n"
      "  frame_duration: 3\n"
      "}\n"
      "container_type: 1\n";
  const char kVideoMediaInfo2[] =
      "video_info {\n"
      "  codec: \"avc1\"\n"
      "  width: 720\n"
      "  height: 480\n"
      "  time_scale: 10\n"
      "  frame_duration: 3\n"
      "}\n"
      "container_type: 1\n";
  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  ASSERT_TRUE(
      adaptation_set->AddRepresentation(ConvertToMediaInfo(kVideoMediaInfo1)));
  ASSERT_TRUE(
      adaptation_set->AddRepresentation(ConvertToMediaInfo(kVideoMediaInfo2)));

  xml::scoped_xml_ptr<xmlNode> adaptation_set_xml(adaptation_set->GetXml());
  EXPECT_THAT(adaptation_set_xml.get(), AttributeEqual("maxFrameRate", "11/3"));
  EXPECT_THAT(adaptation_set_xml.get(), Not(AttributeSet("frameRate")));
}

// Attribute values that are common to all the children Representations should
// propagate up to AdaptationSet. Otherwise, each Representation should have
// its own values.
TEST_F(AdaptationSetTest, BubbleUpAttributesToAdaptationSet) {
  const char k1080p[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width: 1920\n"
      "  height: 1080\n"
      "  time_scale: 30\n"
      "  frame_duration: 1\n"
      "  pixel_width: 1\n"
      "  pixel_height: 1\n"
      "}\n"
      "container_type: 1\n";

  // Different width from the one above.
  const char kDifferentWidth[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width: 1080\n"
      "  height: 1080\n"
      "  time_scale: 30\n"
      "  frame_duration: 1\n"
      "  pixel_width: 1\n"
      "  pixel_height: 1\n"
      "}\n"
      "container_type: 1\n";

  // Different height from ones above
  const char kDifferentHeight[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width: 1440\n"
      "  height: 900\n"
      "  time_scale: 30\n"
      "  frame_duration: 1\n"
      "  pixel_width: 1\n"
      "  pixel_height: 1\n"
      "}\n"
      "container_type: 1\n";

  const char kDifferentFrameRate[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width: 1920\n"
      "  height: 1080\n"
      "  time_scale: 15\n"
      "  frame_duration: 1\n"
      "  pixel_width: 1\n"
      "  pixel_height: 1\n"
      "}\n"
      "container_type: 1\n";

  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  ASSERT_TRUE(adaptation_set->AddRepresentation(ConvertToMediaInfo(k1080p)));

  xml::scoped_xml_ptr<xmlNode> all_attributes_on_adaptation_set(
      adaptation_set->GetXml());
  EXPECT_THAT(all_attributes_on_adaptation_set.get(),
              AttributeEqual("width", "1920"));
  EXPECT_THAT(all_attributes_on_adaptation_set.get(),
              AttributeEqual("height", "1080"));
  EXPECT_THAT(all_attributes_on_adaptation_set.get(),
              AttributeEqual("frameRate", "30/1"));

  ASSERT_TRUE(
      adaptation_set->AddRepresentation(ConvertToMediaInfo(kDifferentWidth)));
  xml::scoped_xml_ptr<xmlNode> width_not_set(adaptation_set->GetXml());
  EXPECT_THAT(width_not_set.get(), Not(AttributeSet("width")));
  EXPECT_THAT(width_not_set.get(), AttributeEqual("height", "1080"));
  EXPECT_THAT(width_not_set.get(), AttributeEqual("frameRate", "30/1"));

  ASSERT_TRUE(
      adaptation_set->AddRepresentation(ConvertToMediaInfo(kDifferentHeight)));
  xml::scoped_xml_ptr<xmlNode> width_height_not_set(adaptation_set->GetXml());
  EXPECT_THAT(width_height_not_set.get(), Not(AttributeSet("width")));
  EXPECT_THAT(width_height_not_set.get(), Not(AttributeSet("height")));
  EXPECT_THAT(width_height_not_set.get(), AttributeEqual("frameRate", "30/1"));

  ASSERT_TRUE(adaptation_set->AddRepresentation(
      ConvertToMediaInfo(kDifferentFrameRate)));
  xml::scoped_xml_ptr<xmlNode> no_common_attributes(adaptation_set->GetXml());
  EXPECT_THAT(no_common_attributes.get(), Not(AttributeSet("width")));
  EXPECT_THAT(no_common_attributes.get(), Not(AttributeSet("height")));
  EXPECT_THAT(no_common_attributes.get(), Not(AttributeSet("frameRate")));
}

TEST_F(AdaptationSetTest, GetRepresentations) {
  const char kMediaInfo1[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width: 720\n"
      "  height: 480\n"
      "  time_scale: 10\n"
      "  frame_duration: 10\n"
      "  pixel_width: 8\n"
      "  pixel_height: 9\n"
      "}\n"
      "container_type: 1\n";
  const char kMediaInfo2[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width: 640\n"
      "  height: 360\n"
      "  time_scale: 10\n"
      "  frame_duration: 10\n"
      "  pixel_width: 1\n"
      "  pixel_height: 1\n"
      "}\n"
      "container_type: 1\n";

  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);

  Representation* representation1 =
      adaptation_set->AddRepresentation(ConvertToMediaInfo(kMediaInfo1));
  EXPECT_THAT(adaptation_set->GetRepresentations(),
              ElementsAre(representation1));

  Representation* representation2 =
      adaptation_set->AddRepresentation(ConvertToMediaInfo(kMediaInfo2));
  EXPECT_THAT(adaptation_set->GetRepresentations(),
              ElementsAre(representation1, representation2));

  auto new_adaptation_set =
      CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  const uint64_t kPresentationTimeOffset = 80;
  Representation* new_representation2 =
      new_adaptation_set->CopyRepresentationWithTimeOffset(
          *representation2, kPresentationTimeOffset);
  Representation* new_representation1 =
      new_adaptation_set->CopyRepresentationWithTimeOffset(
          *representation1, kPresentationTimeOffset);

  EXPECT_THAT(new_adaptation_set->GetRepresentations(),
              // Elements are ordered by id().
              ElementsAre(new_representation1, new_representation2));
}

// Verify that subsegmentAlignment is set to true if all the Representations'
// segments are aligned and the DASH profile is OnDemand.
// Also checking that not all Representations have to be added before calling
// AddNewSegment() on a Representation.
TEST_F(OnDemandAdaptationSetTest, SubsegmentAlignment) {
  const char k480pMediaInfo[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width: 720\n"
      "  height: 480\n"
      "  time_scale: 10\n"
      "  frame_duration: 10\n"
      "  pixel_width: 8\n"
      "  pixel_height: 9\n"
      "}\n"
      "container_type: 1\n";
  const char k360pMediaInfo[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width: 640\n"
      "  height: 360\n"
      "  time_scale: 10\n"
      "  frame_duration: 10\n"
      "  pixel_width: 1\n"
      "  pixel_height: 1\n"
      "}\n"
      "container_type: 1\n";

  // First use same start time and duration, and verify that subsegmentAlignment
  // is set to true.
  const uint64_t kStartTime = 0u;
  const uint64_t kDuration = 10u;
  const uint64_t kAnySize = 19834u;

  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  Representation* representation_480p =
      adaptation_set->AddRepresentation(ConvertToMediaInfo(k480pMediaInfo));
  // Add a subsegment immediately before adding the 360p Representation.
  // This should still work for VOD.
  representation_480p->AddNewSegment(kStartTime, kDuration, kAnySize);

  Representation* representation_360p =
      adaptation_set->AddRepresentation(ConvertToMediaInfo(k360pMediaInfo));
  representation_360p->AddNewSegment(kStartTime, kDuration, kAnySize);

  xml::scoped_xml_ptr<xmlNode> aligned(adaptation_set->GetXml());
  EXPECT_THAT(aligned.get(), AttributeEqual("subsegmentAlignment", "true"));

  // Unknown because 480p has an extra subsegments.
  representation_480p->AddNewSegment(11, 20, kAnySize);
  xml::scoped_xml_ptr<xmlNode> alignment_unknown(adaptation_set->GetXml());
  EXPECT_THAT(alignment_unknown.get(),
              Not(AttributeSet("subsegmentAlignment")));

  // Add segments that make them not aligned.
  representation_360p->AddNewSegment(10, 1, kAnySize);
  representation_360p->AddNewSegment(11, 19, kAnySize);

  xml::scoped_xml_ptr<xmlNode> unaligned(adaptation_set->GetXml());
  EXPECT_THAT(unaligned.get(), Not(AttributeSet("subsegmentAlignment")));
}

// Verify that subsegmentAlignment can be force set to true.
TEST_F(OnDemandAdaptationSetTest, ForceSetsubsegmentAlignment) {
  const char k480pMediaInfo[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width: 720\n"
      "  height: 480\n"
      "  time_scale: 10\n"
      "  frame_duration: 10\n"
      "  pixel_width: 8\n"
      "  pixel_height: 9\n"
      "}\n"
      "container_type: 1\n";
  const char k360pMediaInfo[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width: 640\n"
      "  height: 360\n"
      "  time_scale: 10\n"
      "  frame_duration: 10\n"
      "  pixel_width: 1\n"
      "  pixel_height: 1\n"
      "}\n"
      "container_type: 1\n";
  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  Representation* representation_480p =
      adaptation_set->AddRepresentation(ConvertToMediaInfo(k480pMediaInfo));
  Representation* representation_360p =
      adaptation_set->AddRepresentation(ConvertToMediaInfo(k360pMediaInfo));

  // Use different starting times to make the segments "not aligned".
  const uint64_t kStartTime1 = 1u;
  const uint64_t kStartTime2 = 2u;
  static_assert(kStartTime1 != kStartTime2, "StartTimesShouldBeDifferent");
  const uint64_t kDuration = 10u;
  const uint64_t kAnySize = 19834u;
  representation_480p->AddNewSegment(kStartTime1, kDuration, kAnySize);
  representation_360p->AddNewSegment(kStartTime2, kDuration, kAnySize);
  xml::scoped_xml_ptr<xmlNode> unaligned(adaptation_set->GetXml());
  EXPECT_THAT(unaligned.get(), Not(AttributeSet("subsegmentAlignment")));

  // Then force set the segment alignment attribute to true.
  adaptation_set->ForceSetSegmentAlignment(true);
  xml::scoped_xml_ptr<xmlNode> aligned(adaptation_set->GetXml());
  EXPECT_THAT(aligned.get(), AttributeEqual("subsegmentAlignment", "true"));
}

// Verify that segmentAlignment is set to true if all the Representations
// segments' are aligned and the DASH profile is Live.
TEST_F(LiveAdaptationSetTest, SegmentAlignment) {
  const char k480pMediaInfo[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width: 720\n"
      "  height: 480\n"
      "  time_scale: 10\n"
      "  frame_duration: 10\n"
      "  pixel_width: 8\n"
      "  pixel_height: 9\n"
      "}\n"
      "container_type: 1\n";
  const char k360pMediaInfo[] =
      "video_info {\n"
      "  codec: 'avc1'\n"
      "  width: 640\n"
      "  height: 360\n"
      "  time_scale: 10\n"
      "  frame_duration: 10\n"
      "  pixel_width: 1\n"
      "  pixel_height: 1\n"
      "}\n"
      "container_type: 1\n";
  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  Representation* representation_480p =
      adaptation_set->AddRepresentation(ConvertToMediaInfo(k480pMediaInfo));
  Representation* representation_360p =
      adaptation_set->AddRepresentation(ConvertToMediaInfo(k360pMediaInfo));

  // First use same start time and duration, and verify that that
  // segmentAlignment is set.
  const uint64_t kStartTime = 0u;
  const uint64_t kDuration = 10u;
  const uint64_t kAnySize = 19834u;
  representation_480p->AddNewSegment(kStartTime, kDuration, kAnySize);
  representation_360p->AddNewSegment(kStartTime, kDuration, kAnySize);
  xml::scoped_xml_ptr<xmlNode> aligned(adaptation_set->GetXml());
  EXPECT_THAT(aligned.get(), AttributeEqual("segmentAlignment", "true"));

  // Add segments that make them not aligned.
  representation_480p->AddNewSegment(11, 20, kAnySize);
  representation_360p->AddNewSegment(10, 1, kAnySize);
  representation_360p->AddNewSegment(11, 19, kAnySize);

  xml::scoped_xml_ptr<xmlNode> unaligned(adaptation_set->GetXml());
  EXPECT_THAT(unaligned.get(), Not(AttributeSet("segmentAlignment")));
}

// Verify that the width and height attribute are set if all the video
// representations have the same width and height.
TEST_F(OnDemandAdaptationSetTest, AdapatationSetWidthAndHeight) {
  // Both 720p.
  const char kVideoMediaInfo1[] =
      "video_info {\n"
      "  codec: \"avc1\"\n"
      "  width: 1280\n"
      "  height: 720\n"
      "  time_scale: 3000\n"
      "  frame_duration: 100\n"
      "}\n"
      "container_type: 1\n";
  const char kVideoMediaInfo2[] =
      "video_info {\n"
      "  codec: \"avc1\"\n"
      "  width: 1280\n"
      "  height: 720\n"
      "  time_scale: 3000\n"
      "  frame_duration: 200\n"
      "}\n"
      "container_type: 1\n";
  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  ASSERT_TRUE(
      adaptation_set->AddRepresentation(ConvertToMediaInfo(kVideoMediaInfo1)));
  ASSERT_TRUE(
      adaptation_set->AddRepresentation(ConvertToMediaInfo(kVideoMediaInfo2)));

  xml::scoped_xml_ptr<xmlNode> adaptation_set_xml(adaptation_set->GetXml());
  EXPECT_THAT(adaptation_set_xml.get(), AttributeEqual("width", "1280"));
  EXPECT_THAT(adaptation_set_xml.get(), AttributeEqual("height", "720"));
  EXPECT_THAT(adaptation_set_xml.get(), Not(AttributeSet("maxWidth")));
  EXPECT_THAT(adaptation_set_xml.get(), Not(AttributeSet("maxHeight")));
}

// Verify that the maxWidth and maxHeight attribute are set if there are
// multiple video resolutions.
TEST_F(OnDemandAdaptationSetTest, AdaptationSetMaxWidthAndMaxHeight) {
  const char kVideoMediaInfo1080p[] =
      "video_info {\n"
      "  codec: \"avc1\"\n"
      "  width: 1920\n"
      "  height: 1080\n"
      "  time_scale: 3000\n"
      "  frame_duration: 100\n"
      "}\n"
      "container_type: 1\n";
  const char kVideoMediaInfo720p[] =
      "video_info {\n"
      "  codec: \"avc1\"\n"
      "  width: 1280\n"
      "  height: 720\n"
      "  time_scale: 3000\n"
      "  frame_duration: 100\n"
      "}\n"
      "container_type: 1\n";
  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  ASSERT_TRUE(adaptation_set->AddRepresentation(
      ConvertToMediaInfo(kVideoMediaInfo1080p)));
  ASSERT_TRUE(adaptation_set->AddRepresentation(
      ConvertToMediaInfo(kVideoMediaInfo720p)));

  xml::scoped_xml_ptr<xmlNode> adaptation_set_xml(adaptation_set->GetXml());
  EXPECT_THAT(adaptation_set_xml.get(), AttributeEqual("maxWidth", "1920"));
  EXPECT_THAT(adaptation_set_xml.get(), AttributeEqual("maxHeight", "1080"));
  EXPECT_THAT(adaptation_set_xml.get(), Not(AttributeSet("width")));
  EXPECT_THAT(adaptation_set_xml.get(), Not(AttributeSet("height")));
}

// Verify that Representation::SetSampleDuration() works by checking that
// AdaptationSet@frameRate is in the XML.
TEST_F(AdaptationSetTest, SetSampleDuration) {
  // Omit frame_duration so that SetSampleDuration() will set a new frameRate.
  const char kVideoMediaInfo[] =
      "video_info {\n"
      "  codec: \"avc1\"\n"
      "  width: 1920\n"
      "  height: 1080\n"
      "  time_scale: 3000\n"
      "}\n"
      "container_type: 1\n";

  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);

  const MediaInfo video_media_info = ConvertToMediaInfo(kVideoMediaInfo);
  Representation* representation =
      adaptation_set->AddRepresentation(video_media_info);
  EXPECT_TRUE(representation->Init());

  xml::scoped_xml_ptr<xmlNode> adaptation_set_xml(adaptation_set->GetXml());
  EXPECT_THAT(adaptation_set_xml.get(), Not(AttributeSet("frameRate")));

  representation->SetSampleDuration(2u);
  adaptation_set_xml = adaptation_set->GetXml();
  EXPECT_THAT(adaptation_set_xml.get(), AttributeEqual("frameRate", "3000/2"));
}

// Verify that AdaptationSet::AddContentProtection() and
// UpdateContentProtectionPssh() works.
TEST_F(AdaptationSetTest, AdaptationSetAddContentProtectionAndUpdate) {
  const char kVideoMediaInfo1080p[] =
      "video_info {\n"
      "  codec: \"avc1\"\n"
      "  width: 1920\n"
      "  height: 1080\n"
      "  time_scale: 3000\n"
      "  frame_duration: 100\n"
      "}\n"
      "container_type: 1\n";
  ContentProtectionElement content_protection;
  content_protection.scheme_id_uri =
      "urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed";
  content_protection.value = "some value";
  Element pssh;
  pssh.name = "cenc:pssh";
  pssh.content = "any value";
  content_protection.subelements.push_back(pssh);

  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  ASSERT_TRUE(adaptation_set->AddRepresentation(
      ConvertToMediaInfo(kVideoMediaInfo1080p)));
  adaptation_set->AddContentProtectionElement(content_protection);

  const char kExpectedOutput1[] =
      "<AdaptationSet id=\"1\" contentType=\"video\" width=\"1920\""
      " height=\"1080\" frameRate=\"3000/100\">"
      "  <ContentProtection"
      "   schemeIdUri=\"urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed\""
      "   value=\"some value\">"
      "    <cenc:pssh>any value</cenc:pssh>"
      "  </ContentProtection>"
      "  <Representation id=\"0\" bandwidth=\"0\" codecs=\"avc1\""
      "   mimeType=\"video/mp4\"/>"
      "</AdaptationSet>";
  EXPECT_THAT(adaptation_set->GetXml().get(), XmlNodeEqual(kExpectedOutput1));

  adaptation_set->UpdateContentProtectionPssh(
      "edef8ba9-79d6-4ace-a3c8-27dcd51d21ed", "new pssh value");
  const char kExpectedOutput2[] =
      "<AdaptationSet id=\"1\" contentType=\"video\" width=\"1920\""
      " height=\"1080\" frameRate=\"3000/100\">"
      "  <ContentProtection"
      "   schemeIdUri=\"urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed\""
      "   value=\"some value\">"
      // TODO(rkuroiwa): Commenting this out for now because we want to remove
      // the PSSH from the MPD. Uncomment this when the player supports updating
      // pssh.
      //"    <cenc:pssh>new pssh value</cenc:pssh>"
      "  </ContentProtection>"
      "  <Representation id=\"0\" bandwidth=\"0\" codecs=\"avc1\""
      "   mimeType=\"video/mp4\"/>"
      "</AdaptationSet>";
  EXPECT_THAT(adaptation_set->GetXml().get(), XmlNodeEqual(kExpectedOutput2));
}

// Verify that if the ContentProtection element for the DRM without <cenc:pssh>
// element is updated via UpdateContentProtectionPssh(), the element gets added.
// TODO(rkuroiwa): Until the player supports PSSH update, we remove the pssh
// element. Rename this test once it is supported.
TEST_F(AdaptationSetTest, UpdateToRemovePsshElement) {
  const char kVideoMediaInfo1080p[] =
      "video_info {\n"
      "  codec: \"avc1\"\n"
      "  width: 1920\n"
      "  height: 1080\n"
      "  time_scale: 3000\n"
      "  frame_duration: 100\n"
      "}\n"
      "container_type: 1\n";
  ContentProtectionElement content_protection;
  content_protection.scheme_id_uri =
      "urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed";
  content_protection.value = "some value";

  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  ASSERT_TRUE(adaptation_set->AddRepresentation(
      ConvertToMediaInfo(kVideoMediaInfo1080p)));
  adaptation_set->AddContentProtectionElement(content_protection);

  const char kExpectedOutput1[] =
      "<AdaptationSet id=\"1\" contentType=\"video\" width=\"1920\""
      " height=\"1080\" frameRate=\"3000/100\">"
      "  <ContentProtection"
      "   schemeIdUri=\"urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed\""
      "   value=\"some value\">"
      "  </ContentProtection>"
      "  <Representation id=\"0\" bandwidth=\"0\" codecs=\"avc1\""
      "   mimeType=\"video/mp4\"/>"
      "</AdaptationSet>";
  EXPECT_THAT(adaptation_set->GetXml().get(), XmlNodeEqual(kExpectedOutput1));

  adaptation_set->UpdateContentProtectionPssh(
      "edef8ba9-79d6-4ace-a3c8-27dcd51d21ed", "added pssh value");
  const char kExpectedOutput2[] =
      "<AdaptationSet id=\"1\" contentType=\"video\" width=\"1920\""
      " height=\"1080\" frameRate=\"3000/100\">"
      "  <ContentProtection"
      "   schemeIdUri=\"urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed\""
      "   value=\"some value\">"
      // TODO(rkuroiwa): Commenting this out for now because we want to remove
      // teh PSSH from the MPD. Uncomment this when the player supports updating
      // pssh.
      //"    <cenc:pssh>added pssh value</cenc:pssh>"
      "  </ContentProtection>"
      "  <Representation id=\"0\" bandwidth=\"0\" codecs=\"avc1\""
      "   mimeType=\"video/mp4\"/>"
      "</AdaptationSet>";
  EXPECT_THAT(adaptation_set->GetXml().get(), XmlNodeEqual(kExpectedOutput2));
}

// MPD schema has strict ordering. AudioChannelConfiguration must appear before
// ContentProtection.
// Also test that Representation::AddContentProtection() works.
TEST_F(OnDemandAdaptationSetTest,
       AudioChannelConfigurationWithContentProtection) {
  const char kTestMediaInfo[] =
      "bandwidth: 195857\n"
      "audio_info {\n"
      "  codec: 'mp4a.40.2'\n"
      "  sampling_frequency: 44100\n"
      "  time_scale: 44100\n"
      "  num_channels: 2\n"
      "}\n"
      "init_range {\n"
      "  begin: 0\n"
      "  end: 863\n"
      "}\n"
      "index_range {\n"
      "  begin: 864\n"
      "  end: 931\n"
      "}\n"
      "media_file_name: 'encrypted_audio.mp4'\n"
      "media_duration_seconds: 24.009434\n"
      "reference_time_scale: 44100\n"
      "container_type: CONTAINER_MP4\n";

  const char kExpectedOutput[] =
      "<AdaptationSet id=\"1\" contentType=\"audio\">"
      "  <Representation id=\"0\" bandwidth=\"195857\" codecs=\"mp4a.40.2\""
      "   mimeType=\"audio/mp4\" audioSamplingRate=\"44100\">"
      "    <AudioChannelConfiguration"
      "     schemeIdUri="
      "      \"urn:mpeg:dash:23003:3:audio_channel_configuration:2011\""
      "     value=\"2\"/>"
      "    <ContentProtection schemeIdUri=\"http://foo.com/\">"
      "      <cenc:pssh>anything</cenc:pssh>"
      "    </ContentProtection>"
      "    <BaseURL>encrypted_audio.mp4</BaseURL>"
      "    <SegmentBase indexRange=\"864-931\" timescale=\"44100\">"
      "      <Initialization range=\"0-863\"/>"
      "    </SegmentBase>"
      "  </Representation>"
      "</AdaptationSet>";

  ContentProtectionElement content_protection;
  content_protection.scheme_id_uri = "http://foo.com/";
  Element pssh;
  pssh.name = "cenc:pssh";
  pssh.content = "anything";
  content_protection.subelements.push_back(pssh);

  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, kNoLanguage);
  Representation* audio_representation =
      adaptation_set->AddRepresentation(ConvertToMediaInfo(kTestMediaInfo));
  ASSERT_TRUE(audio_representation);
  audio_representation->AddContentProtectionElement(content_protection);
  EXPECT_THAT(adaptation_set->GetXml().get(), XmlNodeEqual(kExpectedOutput));
}

// Verify that a text path works.
TEST_F(OnDemandAdaptationSetTest, Text) {
  const char kTextMediaInfo[] =
      "text_info {\n"
      "  format: 'ttml'\n"
      "  language: 'en'\n"
      "  type: SUBTITLE\n"
      "}\n"
      "media_duration_seconds: 35\n"
      "bandwidth: 1000\n"
      "media_file_name: 'subtitle.xml'\n"
      "container_type: CONTAINER_TEXT\n";

  const char kExpectedOutput[] =
      "<AdaptationSet id=\"1\" contentType=\"text\" lang=\"en\">"
      "  <Role schemeIdUri=\"urn:mpeg:dash:role:2011\""
      "   value=\"subtitle\"/>\n"
      "  <Representation id=\"0\" bandwidth=\"1000\""
      "   mimeType=\"application/ttml+xml\">"
      "    <BaseURL>subtitle.xml</BaseURL>"
      "  </Representation>"
      "</AdaptationSet>";

  auto adaptation_set = CreateAdaptationSet(kAnyAdaptationSetId, "en");
  Representation* text_representation =
      adaptation_set->AddRepresentation(ConvertToMediaInfo(kTextMediaInfo));
  ASSERT_TRUE(text_representation);

  EXPECT_THAT(adaptation_set->GetXml().get(), XmlNodeEqual(kExpectedOutput));
}

}  // namespace shaka
