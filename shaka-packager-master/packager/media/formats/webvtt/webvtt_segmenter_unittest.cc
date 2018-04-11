// Copyright 2017 Google Inc. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "packager/media/base/media_handler_test_base.h"
#include "packager/media/formats/webvtt/webvtt_segmenter.h"
#include "packager/status_test_util.h"

namespace shaka {
namespace media {

namespace {
const int64_t kStartTimeSigned = 0;
const uint64_t kStartTime = 0;
const int64_t kSegmentDuration = 10000;  // 10 seconds

const size_t kStreamIndex = 0;

const size_t kInputCount = 1;
const size_t kOutputCount = 1;
const size_t kInputIndex = 0;
const size_t kOutputIndex = 0;

const bool kEncrypted = true;
const bool kSubSegment = true;

const char* kId[] = {"cue 1 id", "cue 2 id"};
const char* kPayload[] = {"cue 1 payload", "cue 2 payload"};

const std::string kNoSettings = "";
}  // namespace

class WebVttSegmenterTest : public MediaHandlerTestBase {
 protected:
  void SetUp() {
    ASSERT_OK(SetUpAndInitializeGraph(
        std::make_shared<WebVttSegmenter>(kSegmentDuration), kInputCount,
        kOutputCount));
  }
};

// When a cue ends on a segment boundry, it does not create a cue with a 0 ms
// duration
// |         |
// |[---A---]|
// |         |
TEST_F(WebVttSegmenterTest, CueEndingOnSegmentStart) {
  const uint64_t kSampleDuration = kSegmentDuration;

  {
    testing::InSequence s;

    EXPECT_CALL(*Output(kOutputIndex), OnProcess(IsStreamInfo(kStreamIndex)));

    // Segment One
    EXPECT_CALL(
        *Output(kOutputIndex),
        OnProcess(IsTextSample(kId[0], kStartTime, kStartTime + kSampleDuration,
                               kNoSettings, kPayload[0])));
    EXPECT_CALL(
        *Output(kOutputIndex),
        OnProcess(IsSegmentInfo(kStreamIndex, kStartTimeSigned,
                                kSegmentDuration, !kSubSegment, !kEncrypted)));
    EXPECT_CALL(*Output(kOutputIndex), OnFlush(kStreamIndex));
  }

  ASSERT_OK(Input(kInputIndex)
                ->Dispatch(StreamData::FromStreamInfo(kStreamIndex,
                                                      GetTextStreamInfo())));
  ASSERT_OK(Input(kInputIndex)
                ->Dispatch(StreamData::FromTextSample(
                    kStreamIndex,
                    GetTextSample(kId[0], kStartTime,
                                  kStartTime + kSampleDuration, kPayload[0]))));
  ASSERT_OK(Input(kInputIndex)->FlushAllDownstreams());
}

// Each cue belongs in its own segment, so before each cue is passed
// downstream, a 'input of segment' message should be passed downstream.
//           |
// [---A---] |
//           | [---B---]
//           |
TEST_F(WebVttSegmenterTest, CreatesSegmentsForCues) {
  // Divide segment duration by 2 so that the sample duration won't be a full
  // segment.
  const uint64_t kSampleDuration = kSegmentDuration / 2;

  {
    testing::InSequence s;

    EXPECT_CALL(*Output(kOutputIndex), OnProcess(IsStreamInfo(kStreamIndex)));

    // Segment One
    EXPECT_CALL(
        *Output(kOutputIndex),
        OnProcess(IsTextSample(kId[0], kStartTime, kStartTime + kSampleDuration,
                               kNoSettings, kPayload[0])));
    EXPECT_CALL(
        *Output(kOutputIndex),
        OnProcess(IsSegmentInfo(kStreamIndex, kStartTimeSigned,
                                kSegmentDuration, !kSubSegment, !kEncrypted)));

    // Segment Two
    EXPECT_CALL(
        *Output(kOutputIndex),
        OnProcess(IsTextSample(kId[1], kStartTime + kSegmentDuration,
                               kStartTime + kSegmentDuration + kSampleDuration,
                               kNoSettings, kPayload[1])));
    EXPECT_CALL(*Output(kOutputIndex),
                OnProcess(IsSegmentInfo(
                    kStreamIndex, kStartTimeSigned + kSegmentDuration,
                    kSegmentDuration, !kSubSegment, !kEncrypted)));

    EXPECT_CALL(*Output(kOutputIndex), OnFlush(kStreamIndex));
  }

  ASSERT_OK(Input(kInputIndex)
                ->Dispatch(StreamData::FromStreamInfo(kStreamIndex,
                                                      GetTextStreamInfo())));
  ASSERT_OK(Input(kInputIndex)
                ->Dispatch(StreamData::FromTextSample(
                    kStreamIndex,
                    GetTextSample(kId[0], kStartTime,
                                  kStartTime + kSampleDuration, kPayload[0]))));
  ASSERT_OK(
      Input(kInputIndex)
          ->Dispatch(StreamData::FromTextSample(
              kStreamIndex,
              GetTextSample(kId[1], kStartTime + kSegmentDuration,
                            kStartTime + kSegmentDuration + kSampleDuration,
                            kPayload[1]))));
  ASSERT_OK(Input(kInputIndex)->FlushAllDownstreams());
}

// [---A---] |          |
//           |          |
//           |          | [---B---]
//           |          |
TEST_F(WebVttSegmenterTest, OutputsEmptySegments) {
  const uint64_t kSampleDuration = kSegmentDuration / 2;

  const int64_t kSegment1Start = kStartTime;
  const int64_t kSegment2Start = kSegment1Start + kSegmentDuration;
  const int64_t kSegment3Start = kSegment2Start + kSegmentDuration;

  {
    testing::InSequence s;

    EXPECT_CALL(*Output(kOutputIndex), OnProcess(IsStreamInfo(kStreamIndex)));

    // Segment One
    EXPECT_CALL(
        *Output(kOutputIndex),
        OnProcess(IsTextSample(kId[0], kStartTime, kStartTime + kSampleDuration,
                               kNoSettings, kPayload[0])));
    EXPECT_CALL(
        *Output(kOutputIndex),
        OnProcess(IsSegmentInfo(kStreamIndex, kSegment1Start, kSegmentDuration,
                                !kSubSegment, !kEncrypted)));

    // Segment Two (empty segment)
    EXPECT_CALL(
        *Output(kOutputIndex),
        OnProcess(IsSegmentInfo(kStreamIndex, kSegment2Start, kSegmentDuration,
                                !kSubSegment, !kEncrypted)));

    // Segment Three
    EXPECT_CALL(*Output(kOutputIndex),
                OnProcess(IsTextSample(
                    kId[1], kStartTime + 2 * kSegmentDuration,
                    kStartTime + 2 * kSegmentDuration + kSampleDuration,
                    kNoSettings, kPayload[1])));
    EXPECT_CALL(
        *Output(kOutputIndex),
        OnProcess(IsSegmentInfo(kStreamIndex, kSegment3Start, kSegmentDuration,
                                !kSubSegment, !kEncrypted)));

    EXPECT_CALL(*Output(kOutputIndex), OnFlush(kStreamIndex));
  }

  ASSERT_OK(Input(kInputIndex)
                ->Dispatch(StreamData::FromStreamInfo(kStreamIndex,
                                                      GetTextStreamInfo())));
  ASSERT_OK(Input(kInputIndex)
                ->Dispatch(StreamData::FromTextSample(
                    kStreamIndex,
                    GetTextSample(kId[0], kStartTime,
                                  kStartTime + kSampleDuration, kPayload[0]))));
  ASSERT_OK(
      Input(kInputIndex)
          ->Dispatch(StreamData::FromTextSample(
              kStreamIndex,
              GetTextSample(kId[1], kStartTime + 2 * kSegmentDuration,
                            kStartTime + 2 * kSegmentDuration + kSampleDuration,
                            kPayload[1]))));
  ASSERT_OK(Input(kInputIndex)->FlushAllDownstreams());
}

// When a cue crossing the segment boundary, the cue should be included in
// both segments.
//              |
//  [-----A-----|---------]
//              |
TEST_F(WebVttSegmenterTest, CueCrossesSegments) {
  const uint64_t kSampleDuration = 2 * kSegmentDuration;

  {
    testing::InSequence s;

    EXPECT_CALL(*Output(kOutputIndex), OnProcess(IsStreamInfo(kStreamIndex)));

    // Segment One
    EXPECT_CALL(
        *Output(kOutputIndex),
        OnProcess(IsTextSample(kId[0], kStartTime, kStartTime + kSampleDuration,
                               kNoSettings, kPayload[0])));
    EXPECT_CALL(
        *Output(kOutputIndex),
        OnProcess(IsSegmentInfo(kStreamIndex, kStartTimeSigned,
                                kSegmentDuration, !kSubSegment, !kEncrypted)));

    // Segment Two
    EXPECT_CALL(
        *Output(kOutputIndex),
        OnProcess(IsTextSample(kId[0], kStartTime, kStartTime + kSampleDuration,
                               kNoSettings, kPayload[0])));
    EXPECT_CALL(*Output(kOutputIndex),
                OnProcess(IsSegmentInfo(
                    kStreamIndex, kStartTimeSigned + kSegmentDuration,
                    kSegmentDuration, !kSubSegment, !kEncrypted)));

    EXPECT_CALL(*Output(kOutputIndex), OnFlush(kStreamIndex));
  }

  ASSERT_OK(Input(kInputIndex)
                ->Dispatch(StreamData::FromStreamInfo(kStreamIndex,
                                                      GetTextStreamInfo())));
  ASSERT_OK(Input(kInputIndex)
                ->Dispatch(StreamData::FromTextSample(
                    kStreamIndex,
                    GetTextSample(kId[0], kStartTime,
                                  kStartTime + kSampleDuration, kPayload[0]))));
  ASSERT_OK(Input(kInputIndex)->FlushAllDownstreams());
}

class WebVttSegmenterOrderTest : public MediaHandlerTestBase {};

TEST_F(WebVttSegmenterOrderTest, PreservesOrder) {
  const size_t kInputs = 1;
  const size_t kOutputs = 1;

  const size_t kInput = 0;
  const size_t kOutput = 0;

  const uint64_t kDuration = 10000;
  const int64_t kSegmentStart1 = 0;
  const int64_t kSegmentStart2 = kDuration;

  ASSERT_OK(SetUpAndInitializeGraph(
      std::make_shared<WebVttSegmenter>(kDuration), kInputs, kOutputs));

  {
    testing::InSequence s;

    EXPECT_CALL(*Output(kOutput), OnProcess(IsStreamInfo(kInput)));

    // Segment One
    EXPECT_CALL(*Output(kOutput),
                OnProcess(IsTextSample("1", 5000u, 8500u, "",
                                       "WebVtt testing Line 1 (5.0 - 8.5)")));
    EXPECT_CALL(*Output(kOutput),
                OnProcess(IsTextSample("2", 5000u, 8500u, "",
                                       "WebVtt testing Line 2 (5.0 - 8.5)")));
    EXPECT_CALL(*Output(kOutput),
                OnProcess(IsTextSample("3", 5000u, 12500u, "",
                                       "WebVtt testing (5.0 - 12.5)")));
    EXPECT_CALL(*Output(kOutput), OnProcess(IsSegmentInfo(
                                      kInput, kSegmentStart1, kSegmentDuration,
                                      !kSubSegment, !kEncrypted)));

    // Segment Two
    EXPECT_CALL(*Output(kOutput),
                OnProcess(IsTextSample("3", 5000u, 12500u, "",
                                       "WebVtt testing (5.0 - 12.5)")));
    EXPECT_CALL(*Output(kOutput), OnProcess(IsSegmentInfo(
                                      kInput, kSegmentStart2, kSegmentDuration,
                                      !kSubSegment, !kEncrypted)));

    EXPECT_CALL(*Output(kOutput), OnFlush(kInput));
  }

  ASSERT_OK(Input(kInput)->Dispatch(
      StreamData::FromStreamInfo(0, GetTextStreamInfo())));
  ASSERT_OK(Input(kInput)->Dispatch(StreamData::FromTextSample(
      kOutput,
      GetTextSample("1", 5000, 8500, "WebVtt testing Line 1 (5.0 - 8.5)"))));
  ASSERT_OK(Input(kInput)->Dispatch(StreamData::FromTextSample(
      kOutput,
      GetTextSample("2", 5000, 8500, "WebVtt testing Line 2 (5.0 - 8.5)"))));
  ASSERT_OK(Input(kInput)->Dispatch(StreamData::FromTextSample(
      kOutput,
      GetTextSample("3", 5000, 12500, "WebVtt testing (5.0 - 12.5)"))));

  ASSERT_OK(Input(kInputIndex)->FlushAllDownstreams());
}

}  // namespace media
}  // namespace shaka
