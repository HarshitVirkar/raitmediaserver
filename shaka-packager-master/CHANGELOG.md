## [2.0.0] - 2018-02-10
### Added
- Enhanced HLS support.
  - Support for attributes RESOLUTION, CHANNELS, AUTOSELECT and DEFAULT.
  - Live and Event playlists.
  - fMP4 in HLS (including byte range support).
  - DRM: Widevine and Fairplay.
  - I-Frame playlist.
- Enhanced subtitle support.
  - Segmented WebVTT in fMP4.
  - Segmented WebVTT in text, for HLS.
- Support generating DASH + HLS manifests simultaneously (#262).
- AC3 / E-AC3 support.
- Experimental multi-period support.
- Raw key multi-key support.
- DASH Trickplay.
- Make fMP4 output CMAF compatible.
- Support for WebM colour element.
- Support skip_encryption stream descriptor fields (#219).
- Improved documentation and tutorials.

### Changed
- Refactored packager code and media pipeline.
- Exposed top level packaging interface.
- Renamed --webm_subsample_encryption flag to --vp9_subsample_encryption flag.
- Deprecated --availability_time_offset flag.

### Fixed
- Write manifests atomically to fix possible truncated manifests seen on clients
  (#186).
- [WebM] Fix live segmenter overflow if longer than two hours (#233).
- Fix a possible interferenace problem when re-using UDP multicast streams in
  different processes (#241).
- Create directories in the output path if not exist (#276).
- Fix order of H265 VPS, SPS, PPS in hvcC box (#297).
- Handle additional unused mdat properly (#298).
- Fix possible incorrect HEVC decoder configuration data (#312).
- Handle varying parameter sets in sample when converting from NAL unit stream
  to byte stream (#327).

## [1.6.2] - 2017-04-18
### Added
- Added an option to keep parameter set NAL units (SPS/PPS for H264,
  SPS/PPS/VPS for H265), which is necessary if the parameter set NAL units
  are varying from frame to frame. The flag is --strip_parameter_set_nalus,
  which is true by default. This addresses #206 (the flag needs to be set to
  false).

### Fixed
- Fixed the problem that sliding window logic is still active with DASH static
  live profile (#218).
- Fixed AAC-HE not correctly signaled in codec string (#225).
- [WebM] Fixed output truncated if using the same file for both input and
  output (#210).
- [WebM] Fixed possible integer overflow in stream duration in MPD manifest
  (#214).

## [1.6.1] - 2017-02-10
### Changed
- Enable --generate_dash_if_iop_compliant_mpd by default. This moves
  ContentProtection element from Representation to AdaptationSet. The feature
  can still be disabled by setting the flag to false if needed.

### Fixed
- MPD duration not set for live profile with static mpd (#201).

## [1.6.0] - 2017-01-13
### Added
- Added support for Windows (both 32-bit and 64-bit are supported).
- Added support for live profile with static mpd by setting flag
  --generate_static_mpd (#142). This allows on demand content to use segment
  template.
- Added support for tagging a specific audio AdaptationSet as the default /
  main AdaptationSet with --default_language flag (#155).
- Added UDP options support: udp://ip:port[?options]. Currently three options
  are supported: reuse=1|0 whether reusing UDP sockets are allowed (#133),
  interface=a.b.c.d interface address, timeout=microseconds for socket timeout.
- Added 4K and 8K encryption support (#163).

### Changed
- [WebM][VP9] Use subsample encryption by default for VP9 per latest WebM spec.
  The feature can be disabled by setting --webm_subsample_encryption=false.
- [WebM] Mimic mp4 behavior: either all the samples in a segment are encrypted
  or all the samples are clear.
- [WebM] Move index segment forward to right after init segment (#159).

### Fixed
- Fixed AdaptationSet switching signalling when
  --generate_dash_if_iop_compliant_mpd is enabled (#156).
- [H.264] Fixed access unit detection problem if there are multiple video slice
  NAL units in the same frame (#134).
- [WebVTT] Detect .webvtt as WebVTT files.
- [WebM] Fixed keyframe detection in BlockGroup for encrypted frames.
- [HLS] Fixed HLS playlist problem when clear lead is set to zero (#169).
- Fixed --version command.

### Deprecated
- Deprecated flag --udp_interface_address. Use udp options instead.
- Deprecated flags --single_segment and --profile. They are now derived from
  the presence of 'segment_template' in stream descriptors.

## [1.5.1] - 2016-07-25
### Added
- Added a runtime flag to use dts in timeline for mp4:
  --mp4_use_decoding_timestamp_in_timeline

### Changed
- Remove restriction that sps:gaps_in_frame_num_value_allowed_flag should be
  0 in h264. Packager should not care about this flag (#126).
- Remove restriction that sample duration cannot be zero. A warning message
  is printed instead (#127).

### Fixed
- Fix text formats (webvtt, ttml) not recognized problem (#130).

## [1.5.0] - 2016-07-12
### Added
- Added TS (output) and HLS (output) with SAMPLE-AES encryption support.
  Note that only H.264 and AAC are supported right now.
- Added support for CENCv3, i.e. 'cbcs', 'cbc1', 'cens' protection schemes.
- Added H.265 support in TS (input) and iso-bmff (input / output).
- Added experimental Opus in iso-bmff support.

### Changed
- Change project name from edash-packager to shaka-packager. Also replaces
  various references of edash in the code accordingly.

## [1.4.1] - 2016-06-23
### Fixed
- [VP9] VPCodecConfiguration box should inherit from FullBox instead of Box.
- [VP9] Fixed 'senc' box generation when encrypting mp4:vp9 with superframe.
- [WebM] Close file before trying to get file size, so the file size can be
  correctly calculated.

### Changed
- [MP4] Ignore unrecognized mp4 boxes instead of error out.

## [1.4.0] - 2016-04-08
### Added
- Added support for MacOSX (#65). Thanks to @nevil.
- Added support for Dolby AC3 and EAC3 Audio in ISO-BMFF (#64).
- Added support for language code with subtags, e.g. por-BR is now supported.
- Added a new optional flag (--include_common_pssh) to widevine encryption
  to include [common system pssh box](https://goo.gl/507mKp) in addition to
  widevine pssh box.
- Improved handling of unescaped NAL units in byte stream (#96).

### Changed
- Changed fixed key encryption to generate
  [common system pssh box](https://goo.gl/507mKp) by default; overridable by
  specifying pssh box(es) explicitly with --pssh flag, which is now optional.
  --pssh should be one or more PSSH boxes instead of just pssh data in hex
  string format if it is specified.
- Improved subsample encryption algorithm for H.264 and H.265. Now only video
  data in slice NALs are encrypted (#40).

### Fixed
- Split AdaptationSets by container and codec in addition to content_type,
  language. AVC/MP4 and VP9/WebM are now put in different AdaptationSets if
  they are packaged together.
- Fixed index range off-by-1 error in WebM DASH manifest (#99).
- Fixed WebM SeekHeader bug that the positions should be relative to the
  Segment payload instead of the start of the file.

## [1.3.1] - 2016-01-22
This release fixes and improves WebM parsing and packaging.
### Added
- Added 'cenc:default_KID' attribute in ContentProtection element for non-MP4,
  i.e. WebM mpd too #69.
- Added WebM content decryption support #72.

### Fixed
- Fixed decoding timestamp always being 0 when trasmuxing from WebM to MP4 #67.
- Improved sample duration computation for WebM content to get rid of possible
  gaps due to accumulated errors #68.
- Fixed possible audio sample loss in WebM parser, which could happen if there
  are audio blocks before the first video block #71.

## [1.3.0] - 2016-01-15
### Added
- Added support for new container format: WebM.
- Added support for new codecs:
  - H265 in ISO-BMFF (H265 in other containers will be added later).
  - VP8, VP9 in WebM and ISO-BMFF (experimental).
  - Opus and Vorbis in WebM.
  - DTS in ISO-BMFF.
- Added Verbose logging through --v or --vmodule command line flags.
- Added Subtitle support for On-Demand: allowing subtitle inputs in webvtt or
  ttml. Support for subtitle inputs in media files will be added later.
- Added version information in generated outputs.

### Changed
- Store Sample Auxiliary Information in Sample Encryption Information ('senc')
  box instead of inside Media Data ('mdat') box.
- Got rid of svn dependencies, now all dependencies are in git repo.
- Switched to boringssl, replacing openssl.

### Fixed (in addition to fix in 1.2.1)
- Fixed issue #55 DASH validation (conformance check) problems.
- Fixed AssetId overflow in classic WVM decryption when AssetId exceeds
  0x8000000.
- Fixed a memory leak due to thread object tracking #61.

## [1.2.1] - 2015-11-18
### Fixed
- Fixed a deadlock in MpdBuilder which could lead to program hang #45
- Fixed a race condition in MpdNotifier which could lead to corrupted mpd #49
- Improved support for WVM files:
  - Support files with no PES stream ID metadata.
  - Support files with multiple audio or video configurations.
- Fixed a race condition when flushing ThreadedIoFile which may cause flush
  to be called before file being written; fixed another race condition in
  ThreadedIoFile if there is an error in reading or writing files.
- Relaxed requirement on reserved bits when parsing AVCC #44
- Fixed stropts.h not found issue in CentOS 7.

## [1.2.0] - 2015-10-01
### Added
- Added [docker](https://www.docker.com/) support. Thanks @leandromoreira.

### Changed
- Improved performance with threaded I/O.
- Disabled gold linker by default, which does not work on Ubuntu 64bit server.
- Delete temperary files created by packager when done.
- Updated MediaInfo file formats.

### Fixed
- Support ISO-BMFF files with trailing 'moov' boxes.
- DASH-IF IOP 3.0 Compliance. Some changes are controlled by flag
  `--generate_dash_if_iop_compliant_mpd`. It is defaulted to false, due to lack
  of player support. Will change the default to true in future releases.
  - Added @contentType to AdaptationSet;
  - For video adaptation sets, added `@maxWidth/@width, @maxHeight/@height,
    @maxFrameRate/@frameRate and @par` attributes;
  - For video representations, added `@frameRate and @sar` attributes;
  - For audio adaptation sets, added `@lang` attribute;
  - For representations with aligned segments/subsegments, added attribute
    `@subSegmentAlignment/@segmentAlignment`;
  - Added cenc:default_KID and cenc:pssh to ContentProtection elements;
  - Moved ContentProtection elements up to AdaptationSet element, controlled by
    `--generate_dash_if_iop_compliant_mpd`;
  - Moved representations encrypted with different keys to different adaptation
    sets, grouped by `@group` attribute, controlled by
    `--generate_dash_if_iop_compliant_mpd`.
- Fixed SSL CA cert issue on CentOS.
- Fixed a couple of packager crashes on invalid inputs.
- Read enough bytes before detecting container type. This fixed MPEG-TS not
  recognized issue on some systems.
- Generate proper tkhd.width and tkhd.height with non-square pixels.
- Support composition offset greater than (1<<31).
- Fixed one-sample fragment issue with generated audio streams.
- Fixed and correct width/height in VisualSampleEntry for streams with cropping.
  This fixes encrypted live playback issue for some resolutions.

## [1.1.0] - 2014-10-14
### Added
- Added timeout support for encryption key request.
- Support mpd generation in packager driver program.
- Support segment template identifier $Time$.
- Support configurable policy in Widevine encryption key request.
- Support key rotation, with configurable crypto_period_duration.
- Support UDP unicast/multicast capture.
- Support auto-determination of SD/HD track based on a configurable flag
  `--max_sd_pixels`.
- Support new input formats:
  - WVM (legacy Widevine format), both encrypted and clear;
  - CENC encrypted ISO-BMFF.

### Changed
- Replaced HappyHttp with curl for http request. Added https support.
- Changed packager driver program to be able to package multiple streams.
- Move source code into packager directory, to make it easier to third_party
  integration.

### Fixed
- Support 64 bit mdat box size.
- Support on 32-bit OS.

## 1.0.0 - 2014-04-21
First public release.

### Added
- Repo management with gclient from Chromium.
- Support input formats: fragmented and non-fragmented ISO-BMFF.
- Support encryption with Widevine license server.
- Support encryption with user supplied encryption keys.
- Added packager driver program.
- Added mpd_generator driver program to generate mpd file from packager generated
  intermediate files.

[2.0.0]: https://github.com/google/shaka-packager/compare/v1.6.2...v2.0.0
[1.6.2]: https://github.com/google/shaka-packager/compare/v1.6.1...v1.6.2
[1.6.1]: https://github.com/google/shaka-packager/compare/v1.6.0...v1.6.1
[1.6.0]: https://github.com/google/shaka-packager/compare/v1.5.1...v1.6.0
[1.5.1]: https://github.com/google/shaka-packager/compare/v1.5.0...v1.5.1
[1.5.0]: https://github.com/google/shaka-packager/compare/v1.4.0...v1.5.0
[1.4.1]: https://github.com/google/shaka-packager/compare/v1.4.0...v1.4.1
[1.4.0]: https://github.com/google/shaka-packager/compare/v1.3.1...v1.4.0
[1.3.1]: https://github.com/google/shaka-packager/compare/v1.3.0...v1.3.1
[1.3.0]: https://github.com/google/shaka-packager/compare/v1.2.0...v1.3.0
[1.2.1]: https://github.com/google/shaka-packager/compare/v1.2.0...v1.2.1
[1.2.0]: https://github.com/google/shaka-packager/compare/v1.1...v1.2.0
[1.1.0]: https://github.com/google/shaka-packager/compare/v1.0...v1.1
