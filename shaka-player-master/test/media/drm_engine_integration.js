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

describe('DrmEngine', function() {
  /** @const */
  var ContentType = shaka.util.ManifestParserUtils.ContentType;

  // These come from Axinom and use the Axinom license server.
  // TODO: Do not rely on third-party services long-term.
  /** @const */
  var videoInitSegmentUri = '/base/test/test/assets/multidrm-video-init.mp4';
  /** @const */
  var videoSegmentUri = '/base/test/test/assets/multidrm-video-segment.mp4';
  /** @const */
  var audioInitSegmentUri = '/base/test/test/assets/multidrm-audio-init.mp4';
  /** @const */
  var audioSegmentUri = '/base/test/test/assets/multidrm-audio-segment.mp4';

  /** @type {!Object.<string, ?shakaExtern.DrmSupportType>} */
  var support = {};

  /** @type {!HTMLVideoElement} */
  var video;
  /** @type {!MediaSource} */
  var mediaSource;
  /** @type {shakaExtern.Manifest} */
  var manifest;

  /** @type {!jasmine.Spy} */
  var onErrorSpy;
  /** @type {!jasmine.Spy} */
  var onKeyStatusSpy;
  /** @type {!jasmine.Spy} */
  var onExpirationSpy;
  /** @type {!jasmine.Spy} */
  var onEventSpy;

  /** @type {!shaka.media.DrmEngine} */
  var drmEngine;
  /** @type {!shaka.media.MediaSourceEngine} */
  var mediaSourceEngine;
  /** @type {!shaka.net.NetworkingEngine} */
  var networkingEngine;
  /** @type {!shaka.util.EventManager} */
  var eventManager;

  /** @type {!ArrayBuffer} */
  var videoInitSegment;
  /** @type {!ArrayBuffer} */
  var audioInitSegment;
  /** @type {!ArrayBuffer} */
  var videoSegment;
  /** @type {!ArrayBuffer} */
  var audioSegment;

  beforeAll(function(done) {
    var supportTest = shaka.media.DrmEngine.probeSupport()
        .then(function(result) { support = result; })
        .catch(fail);

    video = /** @type {!HTMLVideoElement} */ (document.createElement('video'));
    video.width = 600;
    video.height = 400;
    video.muted = true;
    document.body.appendChild(video);

    Promise.all([
      supportTest,
      shaka.test.Util.fetch(videoInitSegmentUri),
      shaka.test.Util.fetch(videoSegmentUri),
      shaka.test.Util.fetch(audioInitSegmentUri),
      shaka.test.Util.fetch(audioSegmentUri)
    ]).then(function(responses) {
      videoInitSegment = responses[1];
      videoSegment = responses[2];
      audioInitSegment = responses[3];
      audioSegment = responses[4];
    }).catch(fail).then(done);
  });

  beforeEach(function(done) {
    onErrorSpy = jasmine.createSpy('onError');
    onKeyStatusSpy = jasmine.createSpy('onKeyStatus');
    onExpirationSpy = jasmine.createSpy('onExpirationUpdated');
    onEventSpy = jasmine.createSpy('onEvent');

    mediaSource = new MediaSource();
    video.src = window.URL.createObjectURL(mediaSource);

    networkingEngine = new shaka.net.NetworkingEngine();
    networkingEngine.registerRequestFilter(function(type, request) {
      if (type != shaka.net.NetworkingEngine.RequestType.LICENSE) return;

      request.headers['X-AxDRM-Message'] = [
        'eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJ2ZXJzaW9uIjoxLCJjb21fa2V5X2lk',
        'IjoiNjllNTQwODgtZTllMC00NTMwLThjMWEtMWViNmRjZDBkMTRlIiwibWVzc2FnZSI6e',
        'yJ0eXBlIjoiZW50aXRsZW1lbnRfbWVzc2FnZSIsImtleXMiOlt7ImlkIjoiNmU1YTFkMj',
        'YtMjc1Ny00N2Q3LTgwNDYtZWFhNWQxZDM0YjVhIn1dfX0.yF7PflOPv9qHnu3ZWJNZ12j',
        'gkqTabmwXbDWk_47tLNE'
      ].join('');
    });

    var playerInterface = {
      netEngine: networkingEngine,
      onError: shaka.test.Util.spyFunc(onErrorSpy),
      onKeyStatus: shaka.test.Util.spyFunc(onKeyStatusSpy),
      onExpirationUpdated: shaka.test.Util.spyFunc(onExpirationSpy),
      onEvent: shaka.test.Util.spyFunc(onEventSpy)
    };

    drmEngine = new shaka.media.DrmEngine(playerInterface);
    var config = {
      retryParameters: shaka.net.NetworkingEngine.defaultRetryParameters(),
      clearKeys: {},
      delayLicenseRequestUntilPlayed: false,
      advanced: {},
      servers: {
        'com.widevine.alpha':
            'https://drm-widevine-licensing.axtest.net/AcquireLicense',
        'com.microsoft.playready':
            'https://drm-playready-licensing.axtest.net/AcquireLicense'
      }
    };
    drmEngine.configure(config);

    manifest = new shaka.test.ManifestGenerator()
      .addPeriod(0)
        .addVariant(0)
          .addDrmInfo('com.widevine.alpha')
          .addDrmInfo('com.microsoft.playready')
          .addVideo(1).mime('video/mp4', 'avc1.640015').encrypted(true)
          .addAudio(2).mime('audio/mp4', 'mp4a.40.2').encrypted(true)
      .build();

    var videoStream = manifest.periods[0].variants[0].video;
    var audioStream = manifest.periods[0].variants[0].audio;

    eventManager = new shaka.util.EventManager();

    eventManager.listen(mediaSource, 'sourceopen', function() {
      eventManager.unlisten(mediaSource, 'sourceopen');
      mediaSourceEngine = new shaka.media.MediaSourceEngine(
          video, mediaSource, null);

      // Create empty object first and initialize the fields through
      // [] to allow field names to be expressions.
      var expectedObject = {};
      expectedObject[ContentType.AUDIO] = audioStream;
      expectedObject[ContentType.VIDEO] = videoStream;
      mediaSourceEngine.init(expectedObject);
      done();
    });
  });

  afterEach(function(done) {
    video.removeAttribute('src');
    video.load();
    Promise.all([
      eventManager.destroy(),
      mediaSourceEngine.destroy(),
      networkingEngine.destroy(),
      drmEngine.destroy()
    ]).then(done);
  });

  afterAll(function() {
    document.body.removeChild(video);
  });

  describe('basic flow', function() {
    drm_it('gets a license and can play encrypted segments',
        checkAndRun((done) => {
          // The error callback should not be invoked.
          onErrorSpy.and.callFake(fail);

          var originalRequest = networkingEngine.request.bind(networkingEngine);
          var requestComplete;
          var requestSpy = jasmine.createSpy('request');
          var requestMade = new shaka.util.PublicPromise();
          requestSpy.and.callFake(function() {
            requestMade.resolve();
            requestComplete = originalRequest.apply(null, arguments);
            return requestComplete;
          });
          networkingEngine.request = shaka.test.Util.spyFunc(requestSpy);

          var encryptedEventSeen = new shaka.util.PublicPromise();
          eventManager.listen(video, 'encrypted', function() {
            encryptedEventSeen.resolve();
          });
          eventManager.listen(video, 'error', function() {
            fail('MediaError code ' + video.error.code);
            var extended = video.error.msExtendedCode;
            if (extended) {
              if (extended < 0) {
                extended += Math.pow(2, 32);
              }
              fail('MediaError msExtendedCode ' + extended.toString(16));
            }
          });

          var keyStatusEventSeen = new shaka.util.PublicPromise();
          onKeyStatusSpy.and.callFake(function() {
            keyStatusEventSeen.resolve();
          });

          drmEngine.init(manifest, /* offline */ false).then(function() {
            return drmEngine.attach(video);
          }).then(function() {
            return mediaSourceEngine.appendBuffer(ContentType.VIDEO,
                                                  videoInitSegment,
                                                  null, null);
          }).then(function() {
            return mediaSourceEngine.appendBuffer(ContentType.AUDIO,
                                                  audioInitSegment,
                                                  null, null);
          }).then(function() {
            return encryptedEventSeen;
          }).then(function() {
            // With PlayReady, a persistent license policy can cause a different
            // chain of events.  In particular, the request is bypassed and we
            // get a usable key right away.
            return Promise.race([requestMade, keyStatusEventSeen]);
          }).then(function() {
            if (requestSpy.calls.count()) {
              // We made a license request.
              // Only one request should have been made.
              expect(requestSpy.calls.count()).toBe(1);
              // So it's reasonable to assume that this requestComplete Promise
              // is waiting on the correct request.
              return requestComplete;
            } else {
              // This was probably a PlayReady persistent license.
            }
          }).then(function() {
            // Some platforms (notably 2017 Tizen TVs) do not fire key status
            // events.
            var keyStatusTimeout = shaka.test.Util.delay(5);
            return Promise.race([keyStatusTimeout, keyStatusEventSeen]);
          }).then(function() {
            var call = onKeyStatusSpy.calls.mostRecent();
            if (call) {
              var map = /** @type {!Object} */ (call.args[0]);
              expect(Object.keys(map).length).not.toBe(0);
              for (var k in map) {
                expect(map[k]).toBe('usable');
              }
            }

            return mediaSourceEngine.appendBuffer(ContentType.VIDEO,
                                                  videoSegment,
                                                  null, null);
          }).then(function() {
            return mediaSourceEngine.appendBuffer(ContentType.AUDIO,
                                                  audioSegment,
                                                  null, null);
          }).then(function() {
            expect(video.buffered.end(0)).toBeGreaterThan(0);
            video.play();
            // Try to play for 5 seconds.
            return shaka.test.Util.delay(5);
          }).then(function() {
            // Something should have played by now.
            expect(video.readyState).toBeGreaterThan(1);
            expect(video.currentTime).toBeGreaterThan(0);
          }).catch(fail).then(done);
        }));
  });  // describe('basic flow')

  /**
   * Before running the test, check if the appropriate keysystems are available.
   * @param {function(function())} test
   * @return {function(function())}
   */
  function checkAndRun(test) {
   return function(done) {
     if (!support['com.widevine.alpha'] &&
         !support['com.microsoft.playready']) {
       pending('Skipping DrmEngine tests.');
     } else {
       test(done);
     }
   };
  }
});
