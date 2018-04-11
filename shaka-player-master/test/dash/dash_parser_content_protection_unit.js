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

// Test DRM-related parsing.
describe('DashParser ContentProtection', function() {
  /** @const */
  var Dash = shaka.test.Dash;

  /**
   * Tests that the parser produces the correct results.
   *
   * @param {function()} done
   * @param {string} manifestText
   * @param {Object} expected A Manifest-like object.  The parser output is
   *   expected to match this.
   * @param {shakaExtern.DashContentProtectionCallback=} opt_callback
   * @param {boolean=} opt_ignoreDrmInfo
   */
  function testDashParser(done, manifestText, expected, opt_callback,
      opt_ignoreDrmInfo) {
    var retry = shaka.net.NetworkingEngine.defaultRetryParameters();
    var netEngine = new shaka.test.FakeNetworkingEngine();
    netEngine.setDefaultText(manifestText);
    var dashParser = new shaka.dash.DashParser();
    var callback = opt_callback || function(node) { return null; };
    var ignoreDrmInfo = opt_ignoreDrmInfo || false;
    dashParser.configure({
      retryParameters: retry,
      dash: {
        clockSyncUri: '',
        customScheme: callback,
        ignoreDrmInfo: ignoreDrmInfo,
        xlinkFailGracefully: false,
        defaultPresentationDelay: 10
      }
    });
    var playerEvents = {
      networkingEngine: netEngine,
      filterNewPeriod: function() {},
      filterAllPeriods: function() {},
      onTimelineRegionAdded: fail,  // Should not have any EventStream elements.
      onEvent: fail,
      onError: fail
    };

    dashParser.start('http://example.com', playerEvents)
        .then(function(actual) { expect(actual).toEqual(expected); })
        .catch(fail)
        .then(done);
  }

  /**
   * Build a simple manifest with ContentProtection lines inserted into the
   * AdaptationSet and each Representation.
   *
   * @param {!Array.<string>} adaptationSetLines
   * @param {!Array.<string>} representation1Lines
   * @param {!Array.<string>} representation2Lines
   * @return {string}
   */
  function buildManifestText(
      adaptationSetLines, representation1Lines, representation2Lines) {
    var template = [
      '<MPD xmlns="urn:mpeg:DASH:schema:MPD:2011"',
      '    xmlns:cenc="urn:mpeg:cenc:2013">',
      '  <Period duration="PT30S">',
      '    <SegmentTemplate media="s.mp4" duration="2" />',
      '    <AdaptationSet mimeType="video/mp4" codecs="avc1.4d401f">',
      '%(adaptationSetLines)s',
      '      <Representation bandwidth="50" width="576" height="432">',
      '%(representation1Lines)s',
      '      </Representation>',
      '      <Representation bandwidth="100" width="576" height="432">',
      '%(representation2Lines)s',
      '      </Representation>',
      '    </AdaptationSet>',
      '  </Period>',
      '</MPD>'
    ].join('\n');
    return sprintf(template, {
      adaptationSetLines: adaptationSetLines.join('\n'),
      representation1Lines: representation1Lines.join('\n'),
      representation2Lines: representation2Lines.join('\n')
    });
  }

  /**
   * Build an expected manifest which checks DRM-related fields.
   *
   * @param {!Array.<!Object>} drmInfos A list of DrmInfo-like objects.
   * @param {number=} opt_numVariants The number of variants, default 2.
   * @return {Object} A Manifest-like object.
   */
  function buildExpectedManifest(drmInfos, opt_numVariants) {
    var numVariants = opt_numVariants || 2;
    var keyIds = [];
    if (drmInfos.length > 0) {
      keyIds = drmInfos[0].sample.keyIds;
    }

    var variants = [];
    for (var i = 0; i < numVariants; i++) {
      var variant = jasmine.objectContaining({
        drmInfos: drmInfos,
        video: jasmine.objectContaining({
          keyId: keyIds[i] || null
        })
      });
      variants.push(variant);
    }

    return jasmine.objectContaining({
      periods: [
        jasmine.objectContaining({
          variants: variants,
          textStreams: []
        })
      ]  // periods
    });
  }

  /**
   * Build an expected DrmInfo based on a key system and optional PSSHs.
   *
   * @param {string} keySystem
   * @param {Array.<string>=} opt_keyIds
   * @param {Array.<string>=} opt_base64Psshs
   * @param {Array.<string>=} opt_initDataKeyIds
   * @return {Object} A DrmInfo-like object.
   */
  function buildDrmInfo(keySystem, opt_keyIds,
      opt_base64Psshs, opt_initDataKeyIds) {
    var base64Psshs = opt_base64Psshs || [];
    var initData = base64Psshs.map(function(base64, index) {
      /** @type {shakaExtern.InitDataOverride} */
      var initData = {
        initDataType: 'cenc',
        initData: shaka.util.Uint8ArrayUtils.fromBase64(base64),
        keyId: opt_initDataKeyIds ? opt_initDataKeyIds[index] : null
      };
      return initData;
    });
    var keyIds = opt_keyIds || [];
    var containing = {keySystem: keySystem, initData: initData, keyIds: keyIds};
    return jasmine.objectContaining(containing);
  }

  it('handles clear content', function(done) {
    var source = buildManifestText([], [], []);
    var expected = buildExpectedManifest([]);
    testDashParser(done, source, expected);
  });

  describe('maps standard scheme IDs', function() {
    /**
     * @param {string} name A name for the test
     * @param {!Array.<string>} uuids DRM scheme UUIDs
     * @param {!Array.<string>} keySystems expected key system IDs
     */
    function testKeySystemMappings(name, uuids, keySystems) {
      it(name, function(done) {
        var adaptationSetLines = uuids.map(function(uri) {
          return sprintf('<ContentProtection schemeIdUri="urn:uuid:%s" />',
                         uri);
        });
        var source = buildManifestText(adaptationSetLines, [], []);
        var drmInfos = keySystems.map(function(keySystem) {
          return buildDrmInfo(keySystem);
        });
        var expected = buildExpectedManifest(drmInfos);
        testDashParser(done, source, expected);
      });
    }

    testKeySystemMappings('for Widevine',
        ['edef8ba9-79d6-4ace-a3c8-27dcd51d21ed'], ['com.widevine.alpha']);
    testKeySystemMappings('for PlayReady',
        ['9a04f079-9840-4286-ab92-e65be0885f95'], ['com.microsoft.playready']);
    testKeySystemMappings('for Adobe Primetime',
        ['f239e769-efa3-4850-9c16-a903c6932efb'], ['com.adobe.primetime']);

    testKeySystemMappings('for multiple DRMs in the specified order',
        [
          'edef8ba9-79d6-4ace-a3c8-27dcd51d21ed',
          '9a04f079-9840-4286-ab92-e65be0885f95'
        ], [
          'com.widevine.alpha',
          'com.microsoft.playready'
        ]);

    testKeySystemMappings('in a case-insensitive way',
        [
          'EDEF8BA9-79D6-4ACE-A3C8-27DCD51D21ED',
          '9A04F079-9840-4286-AB92-E65BE0885F95',
          'F239E769-EFA3-4850-9C16-A903C6932EFB'
        ], [
          'com.widevine.alpha',
          'com.microsoft.playready',
          'com.adobe.primetime'
        ]);
  });

  it('inherits key IDs from AdaptationSet to Representation', function(done) {
    var source = buildManifestText([
      // AdaptationSet lines
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed" />',
      '<ContentProtection',
      '  schemeIdUri="urn:mpeg:dash:mp4protection:2011" value="cenc"',
      '  cenc:default_KID="DEADBEEF-FEED-BAAD-F00D-000008675309" />'
    ], [], []);
    var expected = buildExpectedManifest([
      buildDrmInfo('com.widevine.alpha', [
        // Representation 1 key ID
        'deadbeeffeedbaadf00d000008675309',
        // Representation 2 key ID
        'deadbeeffeedbaadf00d000008675309',
      ])
    ]);
    testDashParser(done, source, expected);
  });

  it('sets key IDs for the init data', function(done) {
    var source = buildManifestText([
      // AdaptationSet lines
    ], [
      // Representation 1 lines
      '<ContentProtection',
      '    schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed"',
      '    value="cenc"',
      '    cenc:default_KID="DEADBEEF-FEED-BAAD-F00D-000008675309">',
      '  <cenc:pssh>bm8gaHVtYW4gY2FuIHJlYWQgYmFzZTY0IGRpcmVjdGx5</cenc:pssh>',
      '</ContentProtection>'
    ], []);

    var expected = buildExpectedManifest([
      buildDrmInfo('com.widevine.alpha',
          ['deadbeeffeedbaadf00d000008675309'], // key Id
          ['bm8gaHVtYW4gY2FuIHJlYWQgYmFzZTY0IGRpcmVjdGx5'], // initData
          ['deadbeeffeedbaadf00d000008675309']) // key Id for initData
    ]);
    testDashParser(done, source, expected);
  });

  it('lets Representations override key IDs', function(done) {
    var source = buildManifestText([
      // AdaptationSet lines
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed" />',
      '<ContentProtection',
      '  schemeIdUri="urn:mpeg:dash:mp4protection:2011" value="cenc"',
      '  cenc:default_KID="DEADBEEF-FEED-BAAD-F00D-000008675309" />'
    ], [
      // Representation 1 lines
      '<ContentProtection',
      '  schemeIdUri="urn:mpeg:dash:mp4protection:2011" value="cenc"',
      '  cenc:default_KID="BAADF00D-FEED-DEAF-BEEF-000004390116" />'
    ], [
      // Representation 2 lines
      '<ContentProtection',
      '  schemeIdUri="urn:mpeg:dash:mp4protection:2011" value="cenc"',
      '  cenc:default_KID="BAADF00D-FEED-DEAF-BEEF-018006492568" />'
    ]);
    var expected = buildExpectedManifest([
      buildDrmInfo('com.widevine.alpha', [
        // Representation 1 key ID
        'baadf00dfeeddeafbeef000004390116',
        // Representation 2 key ID
        'baadf00dfeeddeafbeef018006492568',
      ])
    ]);
    testDashParser(done, source, expected);
  });

  it('extracts embedded PSSHs', function(done) {
    var source = buildManifestText([
      // AdaptationSet lines
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed">',
      '  <cenc:pssh>ZmFrZSBXaWRldmluZSBQU1NI</cenc:pssh>',
      '</ContentProtection>',
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:9a04f079-9840-4286-ab92-e65be0885f95">',
      '  <cenc:pssh>bm8gaHVtYW4gY2FuIHJlYWQgYmFzZTY0IGRpcmVjdGx5</cenc:pssh>',
      '</ContentProtection>'
    ], [], []);
    var expected = buildExpectedManifest([
      buildDrmInfo('com.widevine.alpha', [], [
        'ZmFrZSBXaWRldmluZSBQU1NI'
      ]),
      buildDrmInfo('com.microsoft.playready', [], [
        'bm8gaHVtYW4gY2FuIHJlYWQgYmFzZTY0IGRpcmVjdGx5'
      ])
    ]);
    testDashParser(done, source, expected);
  });

  it('assumes all known key systems for generic CENC', function(done) {
    var source = buildManifestText([
      // AdaptationSet lines
      '<ContentProtection',
      '  schemeIdUri="urn:mpeg:dash:mp4protection:2011" value="cenc" />'
    ], [], []);
    var expected = buildExpectedManifest(
        // The order does not matter here, so use arrayContaining.
        /** @type {!Array.<!Object>} */(jasmine.arrayContaining([
          buildDrmInfo('com.widevine.alpha'),
          buildDrmInfo('com.microsoft.playready'),
          buildDrmInfo('com.adobe.primetime')
        ])));
    testDashParser(done, source, expected);
  });

  it('assumes all known key systems when ignoreDrmInfo flag is set',
      function(done) {
        var source = buildManifestText([
          // AdaptationSet lines
          '<ContentProtection',
          '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed">',
          '  <cenc:pssh>ZmFrZSBXaWRldmluZSBQU1NI</cenc:pssh>',
          '</ContentProtection>',
          '<ContentProtection',
          '  schemeIdUri="urn:uuid:9a04f079-9840-4286-ab92-e65be0885f95">',
          '  <cenc:pssh>bm8gaHVtYW4gY2FuIHJlYWQgYmFzZTY0IGRpcm</cenc:pssh>',
          '</ContentProtection>'
        ], [], []);


        var expected = buildExpectedManifest(
            // The order does not matter here, so use arrayContaining.
            // NOTE: the buildDrmInfo calls here specify no init data
            /** @type {!Array.<!Object>} */(jasmine.arrayContaining([
              buildDrmInfo('com.widevine.alpha'),
              buildDrmInfo('com.microsoft.playready'),
              buildDrmInfo('com.adobe.primetime')
            ])));
        testDashParser(done, source, expected, /* opt_callback */ undefined,
                       /* opt_ignoreDrmInfo */ true);
      });

  it('parses key IDs when ignoreDrmInfo flag is set', function(done) {
    var source = buildManifestText([
      // AdaptationSet lines
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:9a04f079-9840-4286-ab92-e65be0885f95"',
      '  cenc:default_KID="DEADBEEF-FEED-BAAD-F00D-000008675309" />',
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed"',
      '  cenc:default_KID="DEADBEEF-FEED-BAAD-F00D-000008675309" />'
    ], [], []);
    var keyIds = [
      // Representation 1 key ID
      'deadbeeffeedbaadf00d000008675309',
      // Representation 2 key ID
      'deadbeeffeedbaadf00d000008675309'
    ];

    var expected = buildExpectedManifest(
        [
          buildDrmInfo('org.w3.clearkey', keyIds),
          buildDrmInfo('com.widevine.alpha', keyIds),
          buildDrmInfo('com.microsoft.playready', keyIds),
          buildDrmInfo('com.adobe.primetime', keyIds)
        ]);
    testDashParser(done, source, expected, /* opt_callback */ undefined,
                   /* opt_ignoreDrmInfo */ true);
  });

  it('inherits PSSH from generic CENC into all key systems', function(done) {
    var source = buildManifestText([
      // AdaptationSet lines
      '<ContentProtection',
      '  schemeIdUri="urn:mpeg:dash:mp4protection:2011" value="cenc">',
      '  <cenc:pssh>b25lIGhlYWRlciB0byBydWxlIHRoZW0gYWxs</cenc:pssh>',
      '</ContentProtection>',
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed" />',
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:9a04f079-9840-4286-ab92-e65be0885f95" />'
    ], [], []);
    var expected = buildExpectedManifest([
      buildDrmInfo('com.widevine.alpha', [], [
        'b25lIGhlYWRlciB0byBydWxlIHRoZW0gYWxs'
      ]),
      buildDrmInfo('com.microsoft.playready', [], [
        'b25lIGhlYWRlciB0byBydWxlIHRoZW0gYWxs'
      ])
    ]);
    testDashParser(done, source, expected);
  });

  it('lets key systems override generic PSSH', function(done) {
    var source = buildManifestText([
      // AdaptationSet lines
      '<ContentProtection',
      '  schemeIdUri="urn:mpeg:dash:mp4protection:2011" value="cenc">',
      '  <cenc:pssh>b25lIGhlYWRlciB0byBydWxlIHRoZW0gYWxs</cenc:pssh>',
      '</ContentProtection>',
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed">',
      '  <cenc:pssh>',
      '    VGltZSBpcyBhbiBpbGx1c2lvbi4gTHVuY2h0aW1lIGRvdWJseSBzby4=',
      '  </cenc:pssh>',
      '</ContentProtection>',
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:9a04f079-9840-4286-ab92-e65be0885f95" />'
    ], [], []);
    var expected = buildExpectedManifest([
      buildDrmInfo('com.widevine.alpha', [], [
        'VGltZSBpcyBhbiBpbGx1c2lvbi4gTHVuY2h0aW1lIGRvdWJseSBzby4='
      ]),
      buildDrmInfo('com.microsoft.playready', [], [
        'b25lIGhlYWRlciB0byBydWxlIHRoZW0gYWxs'
      ])
    ]);
    testDashParser(done, source, expected);
  });

  it('ignores custom or unknown schemes', function(done) {
    var source = buildManifestText([
      // AdaptationSet lines
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:feedbaad-f00d-2bee-baad-d00d00000000" />',
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed" />',
      '<ContentProtection',
      '  schemeIdUri="http://example.com/drm" />'
    ], [], []);
    var expected = buildExpectedManifest([
      buildDrmInfo('com.widevine.alpha')
    ]);
    testDashParser(done, source, expected);
  });

  it('invokes a callback for unknown schemes', function(done) {
    var source = buildManifestText([
      // AdaptationSet lines
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:feedbaad-f00d-2bee-baad-d00d00000000" />',
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed" />',
      '<ContentProtection',
      '  schemeIdUri="http://example.com/drm" />'
    ], [], []);

    /**
     * @param {!Element} contentProtection
     * @return {Array.<shakaExtern.DrmInfo>}
     */
    var callback = function(contentProtection) {
      var schemeIdUri = contentProtection.getAttribute('schemeIdUri');
      if (schemeIdUri == 'urn:uuid:feedbaad-f00d-2bee-baad-d00d00000000') {
        return [{
          keySystem: 'com.custom.baadd00d',
          licenseServerUri: '',
          distinctiveIdentifierRequired: false,
          persistentStateRequired: false,
          videoRobustness: '',
          audioRobustness: '',
          serverCertificate: null,
          initData: [],
          keyIds: []
        }];
      } else if (schemeIdUri == 'http://example.com/drm') {
        return [{
          keySystem: 'com.example.drm',
          licenseServerUri: '',
          distinctiveIdentifierRequired: false,
          persistentStateRequired: false,
          videoRobustness: '',
          audioRobustness: '',
          serverCertificate: null,
          initData: [],
          keyIds: []
        }];
      } else {
        return null;
      }
    };

    var expected = buildExpectedManifest([
      buildDrmInfo('com.custom.baadd00d'),
      buildDrmInfo('com.widevine.alpha'),
      buildDrmInfo('com.example.drm')
    ]);

    testDashParser(done, source, expected, callback);
  });

  it('inserts a placeholder for unrecognized schemes', function(done) {
    var source = buildManifestText([
      // AdaptationSet lines
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:feedbaad-f00d-2bee-baad-d00d00000000" />',
      '<ContentProtection',
      '  schemeIdUri="http://example.com/drm" />',
      '<ContentProtection',
      '  schemeIdUri="urn:mpeg:dash:mp4protection:2011" value="cenc"',
      '  cenc:default_KID="DEADBEEF-FEED-BAAD-F00D-000008675309" />'
    ], [], []);
    var expected = buildExpectedManifest([
      buildDrmInfo('', // placeholder: only unrecognized schemes found
        [
          // Representation 1 key ID
          'deadbeeffeedbaadf00d000008675309',
          // Representation 2 key ID
          'deadbeeffeedbaadf00d000008675309',
        ])
    ]);
    testDashParser(done, source, expected);
  });

  it('can specify ContentProtection in Representation only', function(done) {
    var source = buildManifestText([
      // AdaptationSet lines
    ], [
      // Representation 1 lines
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed" />'
    ], [
      // Representation 2 lines
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed" />'
    ]);
    var expected = buildExpectedManifest(
        [buildDrmInfo('com.widevine.alpha')]);
    testDashParser(done, source, expected);
  });

  it('only keeps key systems common to all Representations', function(done) {
    var source = buildManifestText([
      // AdaptationSet lines
    ], [
      // Representation 1 lines
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:9a04f079-9840-4286-ab92-e65be0885f95" />',
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed" />'
    ], [
      // Representation 2 lines
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed" />'
    ]);
    var expected = buildExpectedManifest(
        [buildDrmInfo('com.widevine.alpha')]);
    testDashParser(done, source, expected);
  });

  it('still keeps per-Representation key IDs when merging', function(done) {
    var source = buildManifestText([
      // AdaptationSet lines
    ], [
      // Representation 1 lines
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed" />',
      '<ContentProtection',
      '  schemeIdUri="urn:mpeg:dash:mp4protection:2011" value="cenc"',
      '  cenc:default_KID="DEADBEEF-FEED-BAAD-F00D-000008675309" />'
    ], [
      // Representation 2 lines
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed" />',
      '<ContentProtection',
      '  schemeIdUri="urn:mpeg:dash:mp4protection:2011" value="cenc"',
      '  cenc:default_KID="BAADF00D-FEED-DEAF-BEEF-000004390116" />'
    ]);
    var expected = buildExpectedManifest([
      buildDrmInfo('com.widevine.alpha', [
        // Representation 1 key ID
        'deadbeeffeedbaadf00d000008675309',
        // Representation 2 key ID
        'baadf00dfeeddeafbeef000004390116',
      ]),
    ]);
    testDashParser(done, source, expected);
  });

  it('parses key IDs from non-cenc in Representation', function(done) {
    var source = buildManifestText([
      // AdaptationSet lines
    ], [
      // Representation 1 lines
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:9a04f079-9840-4286-ab92-e65be0885f95"',
      '  cenc:default_KID="DEADBEEF-FEED-BAAD-F00D-000008675309" />',
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed"',
      '  cenc:default_KID="DEADBEEF-FEED-BAAD-F00D-000008675309" />'
    ], [
      // Representation 2 lines
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:9a04f079-9840-4286-ab92-e65be0885f95"',
      '  cenc:default_KID="BAADF00D-FEED-DEAF-BEEF-000004390116" />',
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed"',
      '  cenc:default_KID="BAADF00D-FEED-DEAF-BEEF-000004390116" />'
    ]);
    var keyIds = [
      // Representation 1 key ID
      'deadbeeffeedbaadf00d000008675309',
      // Representation 2 key ID
      'baadf00dfeeddeafbeef000004390116',
    ];
    var expected = buildExpectedManifest(
        [
          buildDrmInfo('com.microsoft.playready', keyIds),
          buildDrmInfo('com.widevine.alpha', keyIds)
        ]);
    testDashParser(done, source, expected);
  });

  it('parses key IDs from non-cenc in AdaptationSet', function(done) {
    var source = buildManifestText([
      // AdaptationSet lines
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:9a04f079-9840-4286-ab92-e65be0885f95"',
      '  cenc:default_KID="DEADBEEF-FEED-BAAD-F00D-000008675309" />',
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed"',
      '  cenc:default_KID="DEADBEEF-FEED-BAAD-F00D-000008675309" />'
    ], [], []);
    var keyIds = [
      // Representation 1 key ID
      'deadbeeffeedbaadf00d000008675309',
      // Representation 2 key ID
      'deadbeeffeedbaadf00d000008675309'
    ];
    var expected = buildExpectedManifest(
        [
          buildDrmInfo('com.microsoft.playready', keyIds),
          buildDrmInfo('com.widevine.alpha', keyIds)
        ]);
    testDashParser(done, source, expected);
  });

  it('ignores elements missing @schemeIdUri', function(done) {
    var source = buildManifestText([
      // AdaptationSet lines
      '<ContentProtection />',
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed" />'
    ], [], []);
    var expected = buildExpectedManifest(
        [buildDrmInfo('com.widevine.alpha')]);
    testDashParser(done, source, expected);
  });

  it('fails for no schemes common', function(done) {
    var source = buildManifestText([
      // AdaptationSet lines
    ], [
      // Representation 1 lines
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:9a04f079-9840-4286-ab92-e65be0885f95" />'
    ], [
      // Representation 2 lines
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed" />'
    ]);
    var expected = new shaka.util.Error(
        shaka.util.Error.Severity.CRITICAL,
        shaka.util.Error.Category.MANIFEST,
        shaka.util.Error.Code.DASH_NO_COMMON_KEY_SYSTEM);
    Dash.testFails(done, source, expected);
  });

  it('fails for invalid PSSH encoding', function(done) {
    var source = buildManifestText([
      // AdaptationSet lines
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed">',
      '  <cenc:pssh>foobar!</cenc:pssh>',
      '</ContentProtection>'
    ], [], []);
    var expected = new shaka.util.Error(
        shaka.util.Error.Severity.CRITICAL,
        shaka.util.Error.Category.MANIFEST,
        shaka.util.Error.Code.DASH_PSSH_BAD_ENCODING);
    Dash.testFails(done, source, expected);
  });

  it('fails for conflicting default key IDs', function(done) {
    var source = buildManifestText([
      // AdaptationSet lines
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:9a04f079-9840-4286-ab92-e65be0885f95"',
      '  cenc:default_KID="DEADBEEF-FEED-BAAD-F00D-000008675309" />',
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed"',
      '  cenc:default_KID="BAADF00D-FEED-DEAF-BEEF-000004390116" />'
    ], [], []);
    var expected = new shaka.util.Error(
        shaka.util.Error.Severity.CRITICAL,
        shaka.util.Error.Category.MANIFEST,
        shaka.util.Error.Code.DASH_CONFLICTING_KEY_IDS);
    Dash.testFails(done, source, expected);
  });

  it('fails for multiple key IDs', function(done) {
    var source = buildManifestText([
      // AdaptationSet lines
      '<ContentProtection',
      '  schemeIdUri="urn:mpeg:dash:mp4protection:2011" value="cenc"',
      '  cenc:default_KID="BAADF00D-FEED-DEAF-BEEF-000004390116 foobar" />',
      '<ContentProtection',
      '  schemeIdUri="urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed" />'
    ], [], []);
    var expected = new shaka.util.Error(
        shaka.util.Error.Severity.CRITICAL,
        shaka.util.Error.Category.MANIFEST,
        shaka.util.Error.Code.DASH_MULTIPLE_KEY_IDS_NOT_SUPPORTED);
    Dash.testFails(done, source, expected);
  });
});
