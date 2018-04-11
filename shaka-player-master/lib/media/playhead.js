/**
 * @license
 * Copyright 2016 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

goog.provide('shaka.media.Playhead');

goog.require('goog.asserts');
goog.require('shaka.log');
goog.require('shaka.media.TimeRangesUtils');
goog.require('shaka.util.EventManager');
goog.require('shaka.util.FakeEvent');
goog.require('shaka.util.IDestroyable');
goog.require('shaka.util.StreamUtils');
goog.require('shaka.util.Timer');



/**
 * Creates a Playhead, which manages the video's current time.
 *
 * The Playhead provides mechanisms for setting the presentation's start time,
 * restricting seeking to valid time ranges, and stopping playback for startup
 * and re- buffering.
 *
 * @param {HTMLMediaElement} video
 * @param {shakaExtern.Manifest} manifest
 * @param {shakaExtern.StreamingConfiguration} config
 * @param {?number} startTime The playhead's initial position in seconds. If
 *   null, defaults to the start of the presentation for VOD and the live-edge
 *   for live.
 * @param {function()} onSeek Called when the user agent seeks to a time within
 *   the presentation timeline.
 * @param {function(!Event)} onEvent Called when an event is raised to be sent
 *   to the application.
 *
 * @constructor
 * @struct
 * @implements {shaka.util.IDestroyable}
 */
shaka.media.Playhead = function(
    video, manifest, config, startTime, onSeek, onEvent) {
  /** @private {HTMLMediaElement} */
  this.video_ = video;

  /** @private {?shakaExtern.Manifest} */
  this.manifest_ = manifest;

  /** @private {?shakaExtern.StreamingConfiguration} */
  this.config_ = config;

  /**
   * The playhead's initial position in seconds, or null if it should
   * automatically be calculated later.
   * @private {?number}
   */
  this.startTime_ = startTime == null ? null :
                    this.clampSeekToDuration_(startTime);

  /** @private {?function()} */
  this.onSeek_ = onSeek;

  /** @private {?function(!Event)} */
  this.onEvent_ = onEvent;

  /** @private {shaka.util.EventManager} */
  this.eventManager_ = new shaka.util.EventManager();

  /** @private {boolean} */
  this.buffering_ = false;

  /** @private {number} */
  this.playbackRate_ = 1;

  /** @private {?shaka.util.Timer} */
  this.trickPlayTimer_ = null;

  /** @private {?shaka.util.Timer} */
  this.gapJumpTimer_ = null;

  /** @private {boolean} */
  this.seekingEventReceived_ = false;

  /**
   * Used to batch up early seeks and delay until video.currentTime is updated.
   *
   * @private {shaka.util.Timer}
   */
  this.earlySeekTimer_ = new shaka.util.Timer(this.onEarlySeek_.bind(this));

  /** @private {number} */
  this.prevReadyState_ = video.readyState;

  /** @private {boolean} */
  this.didFireLargeGap_ = false;

  /**
   * The wall-clock time (in milliseconds) that the stall occurred.  This is
   * used to ensure we don't flush the pipeline too often.
   * @private {number}
   */
  this.stallWallTime_ = -1;

  /**
   * The playhead time where we think a stall occurred.  When the ready state
   * says we don't have enough data and the playhead stops too long, we assume
   * we have stalled.
   * @private {number}
   */
  this.stallPlayheadTime_ = -1;

  /**
   * True if we have already flushed the pipeline at stallPlayheadTime_.
   * Allows us to avoid flushing multiple times for the same stall.
   * @private {boolean}
   */
  this.stallCorrected_ = false;

  /** @private {boolean} */
  this.hadSegmentAppended_ = false;


  // Check if the video has already loaded some metadata.
  if (video.readyState > 0) {
    this.onLoadedMetadata_();
  } else {
    this.eventManager_.listenOnce(
        video, 'loadedmetadata', this.onLoadedMetadata_.bind(this));

    // Check for early seeks before we have any media loaded.
    // See onEarlySeek_() for details.
    this.eventManager_.listen(video, 'timeupdate', function() {
      // In practice, the value of video.currentTime doesn't change before the
      // event is fired.  Delay 100ms and batch up changes.
      this.earlySeekTimer_.schedule(0.1 /* seconds */);
    }.bind(this));
  }

  var pollGap = this.onPollGapJump_.bind(this);
  this.eventManager_.listen(video, 'ratechange', this.onRateChange_.bind(this));
  this.eventManager_.listen(video, 'waiting', pollGap);

  // We can't trust readyState or 'waiting' events on all platforms.  So poll
  // the current time and if we are in a gap, jump it.
  // See: https://goo.gl/sbSHp9 and https://goo.gl/cuAcYd
  this.gapJumpTimer_ = new shaka.util.Timer(pollGap);
  this.gapJumpTimer_.scheduleRepeated(0.25);
};


/** @override */
shaka.media.Playhead.prototype.destroy = function() {
  var p = this.eventManager_.destroy();
  this.eventManager_ = null;

  if (this.trickPlayTimer_ != null) {
    this.trickPlayTimer_.cancel();
    this.trickPlayTimer_ = null;
  }

  if (this.gapJumpTimer_ != null) {
    this.gapJumpTimer_.cancel();
    this.gapJumpTimer_ = null;
  }

  if (this.earlySeekTimer_ != null) {
    this.earlySeekTimer_.cancel();
    this.earlySeekTimer_ = null;
  }

  this.video_ = null;
  this.manifest_ = null;
  this.config_ = null;
  this.onSeek_ = null;
  this.onEvent_ = null;

  return p;
};


/**
 * Adjust the start time.  Used by Player to implement the
 * streaming.startAtSegmentBoundary configuration.
 *
 * @param {number} startTime
 */
shaka.media.Playhead.prototype.setStartTime = function(startTime) {
  if (this.video_.readyState > 0)
    this.video_.currentTime = this.clampTime_(startTime);
  else
    this.startTime_ = startTime;
};


/**
 * Clamp seek times and playback start times so that we never seek to the
 * presentation duration.  Seeking to or starting at duration does not work
 * consistently across browsers.
 *
 * TODO: Clean up and simplify Playhead.  There are too many layers of, methods
 * for, and conditions on timestamp adjustment.
 *
 * @see https://github.com/google/shaka-player/issues/979
 * @param {number} time
 * @return {number} The adjusted seek time.
 * @private
 */
shaka.media.Playhead.prototype.clampSeekToDuration_ = function(time) {
  var timeline = this.manifest_.presentationTimeline;
  var duration = timeline.getDuration();
  if (time >= duration) {
    goog.asserts.assert(this.config_.durationBackoff >= 0,
                        'Duration backoff must be non-negative!');
    return duration - this.config_.durationBackoff;
  }
  return time;
};


/**
 * Gets the playhead's current (logical) position.
 *
 * @return {number}
 */
shaka.media.Playhead.prototype.getTime = function() {
  if (this.video_.readyState > 0) {
    // We don't buffer when the livestream video is paused and the playhead time
    // is out of the seek range, thus we do not clamp the current time when the
    // video is paused.
    // https://github.com/google/shaka-player/issues/1121
    if (this.video_.paused) return this.video_.currentTime;

    // Although we restrict the video's currentTime elsewhere, clamp it here to
    // ensure any timing issues (e.g., the user agent seeks and calls this
    // function before we receive the 'seeking' event) don't cause us to return
    // a time outside the segment availability window.
    return this.clampTime_(this.video_.currentTime);
  }

  return this.getStartTime_();
};


/**
 * Gets the playhead's initial position in seconds.
 *
 * @return {number}
 * @private
 */
shaka.media.Playhead.prototype.getStartTime_ = function() {
  if (this.startTime_ != null) {
    return this.clampTime_(this.startTime_);
  }

  var startTime;
  var timeline = this.manifest_.presentationTimeline;
  if (timeline.getDuration() < Infinity) {
    // If the presentation is VOD, or if the presentation is live but has
    // finished broadcasting, then start from the beginning.
    startTime = timeline.getSegmentAvailabilityStart();
  } else {
    // Otherwise, start near the live-edge.
    startTime = timeline.getSeekRangeEnd();
  }

  // If we don't set this, getStartTime_ will continue to wander forward.  For
  // a drifting stream, this is very bad, as we will never fall back into a
  // playable range.
  // TODO: re-evaluate after #999 (drift tolerance refactor) is resolved
  this.startTime_ = this.clampSeekToDuration_(startTime);

  return startTime;
};


/**
 * Stops the playhead for buffering, or resumes the playhead after buffering.
 *
 * @param {boolean} buffering True to stop the playhead; false to allow it to
 *   continue.
 */
shaka.media.Playhead.prototype.setBuffering = function(buffering) {
  if (buffering != this.buffering_) {
    this.buffering_ = buffering;
    this.setPlaybackRate(this.playbackRate_);
  }
};


/**
 * Gets the current effective playback rate.  This may be negative even if the
 * browser does not directly support rewinding.
 * @return {number}
 */
shaka.media.Playhead.prototype.getPlaybackRate = function() {
  return this.playbackRate_;
};


/**
 * Sets the playback rate.
 * @param {number} rate
 */
shaka.media.Playhead.prototype.setPlaybackRate = function(rate) {
  if (this.trickPlayTimer_ != null) {
    this.trickPlayTimer_.cancel();
    this.trickPlayTimer_ = null;
  }

  this.playbackRate_ = rate;
  // All major browsers support playback rates above zero.  Only need fake
  // trick play for negative rates.
  this.video_.playbackRate = (this.buffering_ || rate < 0) ? 0 : rate;

  if (!this.buffering_ && rate < 0) {
    // Defer creating the timer until we stop buffering.  This function will be
    // called again from setBuffering().
    var trickPlay = (function() {
      this.video_.currentTime += rate / 4;
    }.bind(this));
    this.trickPlayTimer_ = new shaka.util.Timer(trickPlay);
    this.trickPlayTimer_.scheduleRepeated(0.25);
  }
};


/**
 * Called when a segment is appended by StreamingEngine, but not when a clear is
 * pending.  This means StreamingEngine will continue buffering forward from
 * what is buffered. So we know about any gaps before the start.
 */
shaka.media.Playhead.prototype.onSegmentAppended = function() {
  this.hadSegmentAppended_ = true;
  this.onPollGapJump_();
};


/**
 * Handles a 'ratechange' event.
 *
 * @private
 */
shaka.media.Playhead.prototype.onRateChange_ = function() {
  // NOTE: This will not allow explicitly setting the playback rate to 0 while
  // the playback rate is negative.  Pause will still work.
  var expectedRate =
      this.buffering_ || this.playbackRate_ < 0 ? 0 : this.playbackRate_;

  // Native controls in Edge trigger a change to playbackRate and set it to 0
  // when seeking.  If we don't exclude 0 from this check, we will force the
  // rate to stay at 0 after a seek with Edge native controls.
  // https://github.com/google/shaka-player/issues/951
  if (this.video_.playbackRate && this.video_.playbackRate != expectedRate) {
    shaka.log.debug('Video playback rate changed to', this.video_.playbackRate);
    this.setPlaybackRate(this.video_.playbackRate);
  }
};


/**
 * Handles a 'loadedmetadata' event.
 *
 * @private
 */
shaka.media.Playhead.prototype.onLoadedMetadata_ = function() {
  // Move the real playhead to the start time.
  var targetTime = this.getStartTime_();

  // We are out of the phase in which "early seek" may occur, so stop listening
  // for the 'timeupdate' event.
  this.eventManager_.unlisten(this.video_, 'timeupdate');

  // Cancel any pending early seek timer.  Cancelation is important because it
  // resolves a race between the first "timeupdate" and the "loadedmetadata"
  // event.  On some browsers, "timeupdate" fires right before "loadedmetadata",
  // which would cause earlySeekTimer_ to fire inappropriately and trigger a
  // failed assertion.
  this.earlySeekTimer_.cancel();

  if (Math.abs(this.video_.currentTime - targetTime) < 0.001) {
    this.eventManager_.listen(
        this.video_, 'seeking', this.onSeeking_.bind(this));
    this.eventManager_.listen(
        this.video_, 'playing', this.onPlaying_.bind(this));
  } else {
    this.eventManager_.listenOnce(
        this.video_, 'seeking', this.onSeekingToStartTime_.bind(this));
    this.video_.currentTime = targetTime;
  }
};


/**
 * Handles the 'seeking' event from the initial jump to the start time (if
 * there is one).
 *
 * @private
 */
shaka.media.Playhead.prototype.onSeekingToStartTime_ = function() {
  goog.asserts.assert(this.video_.readyState > 0,
                      'readyState should be greater than 0');
  this.eventManager_.listen(this.video_, 'seeking', this.onSeeking_.bind(this));
  this.eventManager_.listen(this.video_, 'playing', this.onPlaying_.bind(this));
};


/**
 * Called on a recurring timer to check for gaps in the media.  This is also
 * called in a 'waiting' event.
 *
 * @private
 */
shaka.media.Playhead.prototype.onPollGapJump_ = function() {
  // Don't gap jump before the video is ready to play.
  if (this.video_.readyState == 0) return;
  // Do not gap jump if seeking has begun, but the seeking event has not
  // yet fired for this particular seek.
  if (this.video_.seeking) {
    if (!this.seekingEventReceived_)
      return;
  } else {
    this.seekingEventReceived_ = false;
  }
  // Don't gap jump while paused, so that you don't constantly jump ahead while
  // paused on a livestream.
  if (this.video_.paused) return;

  // When the ready state changes, we have moved on, so we should fire the large
  // gap event if we see one.
  if (this.video_.readyState != this.prevReadyState_) {
    this.didFireLargeGap_ = false;
    this.prevReadyState_ = this.video_.readyState;
  }

  var smallGapLimit = this.config_.smallGapLimit;
  var currentTime = this.video_.currentTime;
  var buffered = this.video_.buffered;

  // If seeking is not possible, clamp the playhead manually here.
  var timeline = this.manifest_.presentationTimeline;
  var availabilityStart = timeline.getSegmentAvailabilityStart();
  if (currentTime < availabilityStart) {
    // The availability window has moved past the playhead.
    // Move ahead to catch up.
    var targetTime = this.reposition_(currentTime);
    shaka.log.info('Jumping forward ' + (targetTime - currentTime) +
        ' seconds to catch up with the availability window.');
    this.movePlayhead_(currentTime, targetTime);
    return;
  }

  var gapIndex = shaka.media.TimeRangesUtils.getGapIndex(buffered, currentTime);

  // The current time is unbuffered or is too far from a gap.
  if (gapIndex == null) {
    if (this.video_.readyState < 3 && this.video_.playbackRate > 0) {
      // Some platforms/browsers can get stuck in the middle of a buffered range
      // (e.g. when seeking in a background tab).  Flush the media pipeline to
      // help.
      //
      // Flush once we have stopped for more than 1 second inside a buffered
      // range.  Note that Chromecast takes a few seconds to start playing
      // after any kind of seek, so wait 5 seconds between repeated flushes.
      if (this.stallPlayheadTime_ != currentTime) {
        this.stallPlayheadTime_ = currentTime;
        this.stallWallTime_ = Date.now();
        this.stallCorrected_ = false;
      } else if (!this.stallCorrected_ &&
                 this.stallWallTime_ < Date.now() - 1000) {
        for (var i = 0; i < buffered.length; i++) {
          // Ignore the end of the buffered range since it may not play any more
          // on all platforms.
          if (currentTime >= buffered.start(i) &&
              currentTime < buffered.end(i) - 0.5) {
            shaka.log.debug(
                'Flushing media pipeline due to stall inside buffered range');
            this.video_.currentTime += 0.1;
            this.stallPlayheadTime_ = this.video_.currentTime;
            this.stallCorrected_ = true;
            break;
          }
        }
      }
    }
    return;
  }
  // If we are before the first buffered range, this could be an unbuffered
  // seek.  So wait until a segment is appended so we are sure it is a gap.
  if (gapIndex == 0 && !this.hadSegmentAppended_)
    return;

  // StreamingEngine can buffer past the seek end, but still don't allow seeking
  // past it.
  var jumpTo = buffered.start(gapIndex);
  var seekEnd = this.manifest_.presentationTimeline.getSeekRangeEnd();
  if (jumpTo >= seekEnd)
    return;

  var jumpSize = jumpTo - currentTime;
  var isGapSmall = jumpSize <= smallGapLimit;
  var jumpLargeGap = false;

  if (!isGapSmall && !this.didFireLargeGap_) {
    this.didFireLargeGap_ = true;

    // Event firing is synchronous.
    var event = new shaka.util.FakeEvent(
        'largegap', {'currentTime': currentTime, 'gapSize': jumpSize});
    event.cancelable = true;
    this.onEvent_(event);

    if (this.config_.jumpLargeGaps && !event.defaultPrevented)
      jumpLargeGap = true;
    else
      shaka.log.info('Ignoring large gap at', currentTime);
  }

  if (isGapSmall || jumpLargeGap) {
    if (gapIndex == 0) {
      shaka.log.info(
          'Jumping forward', jumpSize,
          'seconds because of gap before start time of', jumpTo);
    } else {
      shaka.log.info(
          'Jumping forward', jumpSize, 'seconds because of gap starting at',
          buffered.end(gapIndex - 1), 'and ending at', jumpTo);
    }

    this.movePlayhead_(currentTime, jumpTo);
  }
};


/**
 * Handles a 'timeupdate' event that occurs before metadata is loaded, which
 * would indicate that the user is seeking.
 *
 * Note that a 'seeking' event will not fire before content is loaded.  In this
 * state, the playhead can only move as a result of a seek action, so timeupdate
 * is a good choice.
 *
 * @private
 */
shaka.media.Playhead.prototype.onEarlySeek_ = function() {
  goog.asserts.assert(this.video_.readyState == 0,
                      'readyState should be 0 for early seeking');

  var currentTime = this.video_.currentTime;
  var targetTime = this.reposition_(currentTime);

  shaka.log.v1('Early seek to', currentTime, 'remapped to', targetTime);
  this.startTime_ = targetTime;
};


/**
 * Handles a 'seeking' event.
 *
 * @private
 */
shaka.media.Playhead.prototype.onSeeking_ = function() {
  goog.asserts.assert(this.video_.readyState > 0,
                      'readyState should be greater than 0');

  this.seekingEventReceived_ = true;
  this.hadSegmentAppended_ = false;
  var currentTime = this.video_.currentTime;
  var targetTime = this.reposition_(currentTime);

  if (Math.abs(targetTime - currentTime) > 0.001) {
    this.movePlayhead_(currentTime, targetTime);
    return;
  }

  shaka.log.v1('Seek to ' + currentTime);
  this.didFireLargeGap_ = false;
  this.onSeek_();
};


/**
 * Handles a 'playing' event.
 *
 * @private
 */
shaka.media.Playhead.prototype.onPlaying_ = function() {
  goog.asserts.assert(this.video_.readyState > 0,
                      'readyState should be greater than 0');

  var currentTime = this.video_.currentTime;
  var targetTime = this.reposition_(currentTime);

  if (Math.abs(targetTime - currentTime) > 0.001)
    this.movePlayhead_(currentTime, targetTime);
};


/**
 * Computes a new playhead position that's within the presentation timeline.
 *
 * @param {number} currentTime
 * @return {number} The time to reposition the playhead to.
 * @private
 */
shaka.media.Playhead.prototype.reposition_ = function(currentTime) {
  goog.asserts.assert(this.manifest_ && this.config_, 'Must not be destroyed');

  /** @type {function(number)} */
  var isBuffered =
      shaka.media.TimeRangesUtils.isBuffered.bind(null, this.video_.buffered);

  var rebufferingGoal = shaka.util.StreamUtils.getRebufferingGoal(
      this.manifest_, this.config_, 1 /* scaleFactor */);

  var timeline = this.manifest_.presentationTimeline;
  var start = timeline.getSafeAvailabilityStart(0);
  var end = timeline.getSegmentAvailabilityEnd();
  var duration = timeline.getDuration();

  // With live content, the beginning of the availability window is moving
  // forward.  This means we cannot seek to it since we will "fall" outside the
  // window while we buffer.  So we define a "safe" region that is far enough
  // away.  For VOD, |safe == start|.
  var safe = timeline.getSafeAvailabilityStart(rebufferingGoal);

  // These are the times to seek to rather than the exact destinations.  When
  // we seek, we will get another event (after a slight delay) and these steps
  // will run again.  So if we seeked directly to |start|, |start| would move
  // on the next call and we would loop forever.
  //
  // Offset by 5 seconds since Chromecast takes a few seconds to start playing
  // after a seek, even when buffered.
  var seekStart = timeline.getSafeAvailabilityStart(5);
  var seekSafe = timeline.getSafeAvailabilityStart(rebufferingGoal + 5);


  if (currentTime >= duration) {
    shaka.log.v1('Playhead past duration.');
    return this.clampSeekToDuration_(currentTime);
  }

  if (currentTime > end) {
    shaka.log.v1('Playhead past end.');
    return end;
  }

  if (currentTime < start) {
    if (isBuffered(seekStart)) {
      shaka.log.v1('Playhead before start & start is buffered');
      return seekStart;
    } else {
      shaka.log.v1('Playhead before start & start is unbuffered');
      return seekSafe;
    }
  }

  if (currentTime >= safe || isBuffered(currentTime)) {
    shaka.log.v1('Playhead in safe region or in buffered region.');
    return currentTime;
  } else {
    shaka.log.v1('Playhead outside safe region & in unbuffered region.');
    return seekSafe;
  }
};


/**
 * Moves the playhead to the target time, triggering a call to onSeeking_().
 *
 * @param {number} currentTime
 * @param {number} targetTime
 * @private
 */
shaka.media.Playhead.prototype.movePlayhead_ = function(
    currentTime, targetTime) {
  shaka.log.debug('Moving playhead...',
                  'currentTime=' + currentTime,
                  'targetTime=' + targetTime);
  this.video_.currentTime = targetTime;

  // Sometimes, IE and Edge ignore re-seeks.  Check every 100ms and try
  // again if need be, up to 10 tries.
  // Delay stats over 100 runs of a re-seeking integration test:
  // IE     -   0ms -  47%
  // IE     - 100ms -  63%
  // Edge   -   0ms -   2%
  // Edge   - 100ms -  40%
  // Edge   - 200ms -  32%
  // Edge   - 300ms -  24%
  // Edge   - 400ms -   2%
  // Chrome -   0ms - 100%
  // TODO: File a bug on IE/Edge about this.
  var tries = 0;
  var recheck = (function() {
    if (!this.video_) return;
    if (tries++ >= 10) return;

    if (this.video_.currentTime == currentTime) {
      // Sigh.  Try again.
      this.video_.currentTime = targetTime;
      setTimeout(recheck, 100);
    }
  }).bind(this);
  setTimeout(recheck, 100);
};


/**
 * Clamps the given time to the segment availability window.
 *
 * @param {number} time The time in seconds.
 * @return {number} The clamped time in seconds.
 * @private
 */
shaka.media.Playhead.prototype.clampTime_ = function(time) {
  var start = this.manifest_.presentationTimeline.getSegmentAvailabilityStart();
  if (time < start) return start;

  var end = this.manifest_.presentationTimeline.getSegmentAvailabilityEnd();
  if (time > end) return end;

  return time;
};
