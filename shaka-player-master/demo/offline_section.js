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

/**
 * @fileoverview Shaka Player demo, main section.
 *
 * @suppress {visibility} to work around compiler errors until we can
 *   refactor the demo into classes that talk via public method.  TODO
 */


/** @suppress {duplicate} */
var shakaDemo = shakaDemo || {};


/** @private {?HTMLOptGroupElement} */
shakaDemo.offlineOptGroup_ = null;


/** @private {boolean} */
shakaDemo.offlineOperationInProgress_ = false;


/**
 * @param {boolean} canHide True to hide the progress value if there isn't an
 *   operation going.
 * @private
 */
shakaDemo.updateButtons_ = function(canHide) {
  var assetList = document.getElementById('assetList');
  var inProgress = shakaDemo.offlineOperationInProgress_;

  document.getElementById('progressDiv').style.display =
      canHide && !inProgress ? 'none' : 'block';

  var option = assetList.options[assetList.selectedIndex];
  var storedContent = option.storedContent;
  // True if there is no DRM or if the browser supports persistent licenses for
  // any given DRM system.
  var supportsDrm = !option.asset || !option.asset.drm ||
      !option.asset.drm.length || option.asset.drm.some(function(drm) {
        return shakaDemo.support_.drm[drm] &&
            shakaDemo.support_.drm[drm].persistentState;
      });

  // Only show when the custom asset option is selected.
  document.getElementById('offlineNameDiv').style.display =
      option.asset ? 'none' : 'block';

  var button = document.getElementById('storeDeleteButton');
  button.disabled = (inProgress || !supportsDrm || option.isStored);
  button.innerText = storedContent ? 'Delete' : 'Store';
  var helpText = document.getElementById('storeDeleteHelpText');
  if (inProgress)
    helpText.textContent = 'Operation is in progress...';
  else if (!supportsDrm)
    helpText.textContent = 'This browser does not support persistent licenses.';
  else if (button.disabled)
    helpText.textContent = 'The asset is stored offline. ' +
        'Checkout the "Offline" section in the "Asset" list';
  else
    helpText.textContent = '';
};


/** @private */
shakaDemo.setupOffline_ = function() {
  document.getElementById('storeDeleteButton')
      .addEventListener('click', shakaDemo.storeDeleteAsset_);
  document.getElementById('assetList')
      .addEventListener('change', shakaDemo.updateButtons_.bind(null, true));
  shakaDemo.updateButtons_(true);
};


/**
 * @return {!Promise}
 * @private
 */
shakaDemo.setupOfflineAssets_ = function() {
  var Storage = shaka.offline.Storage;
  if (!Storage.support()) {
    var section = document.getElementById('offlineSection');
    section.style.display = 'none';
    return Promise.resolve();
  }

  /** @type {!HTMLOptGroupElement} */
  var group;
  var assetList = document.getElementById('assetList');
  if (!shakaDemo.offlineOptGroup_) {
    group =
        /** @type {!HTMLOptGroupElement} */ (
            document.createElement('optgroup'));
    shakaDemo.offlineOptGroup_ = group;
    group.label = 'Offline';
    assetList.appendChild(group);
  } else {
    group = shakaDemo.offlineOptGroup_;
  }

  var db = new Storage(shakaDemo.localPlayer_);
  return db.list().then(function(storedContents) {
    storedContents.forEach(function(storedContent) {
      for (var i = 0; i < assetList.options.length; i++) {
        var option = assetList.options[i];
        if (option.asset &&
            option.asset.manifestUri == storedContent.originalManifestUri) {
          option.isStored = true;
          break;
        }
      }
      var asset = {manifestUri: storedContent.offlineUri};

      var option = document.createElement('option');
      option.textContent =
          storedContent.appMetadata ? storedContent.appMetadata.name : '';
      option.asset = asset;
      option.storedContent = storedContent;
      group.appendChild(option);
    });

    shakaDemo.updateButtons_(true);
    return db.destroy();
  }).catch(function(error) {
    if (error.code == shaka.util.Error.Code.UNSUPPORTED_UPGRADE_REQUEST) {
      console.warn('Warning: storage cleared.  For details, see ' +
                   'https://github.com/google/shaka-player/issues/1248');
      shaka.offline.Storage.deleteAll();
      return;
    }

    // Let another component deal with it.
    throw error;
  });
};


/** @private */
shakaDemo.storeDeleteAsset_ = function() {
  shakaDemo.closeError();
  shakaDemo.offlineOperationInProgress_ = true;
  shakaDemo.updateButtons_(false);

  var assetList = document.getElementById('assetList');
  var progress = document.getElementById('progress');
  var option = assetList.options[assetList.selectedIndex];

  progress.textContent = '0';

  var storage = new shaka.offline.Storage(shakaDemo.localPlayer_);
  storage.configure(/** @type {shakaExtern.OfflineConfiguration} */ ({
    progressCallback: function(data, percent) {
      progress.textContent = (percent * 100).toFixed(2);
    }
  }));

  var p;
  if (option.storedContent) {
    var offlineUri = option.storedContent.offlineUri;
    var originalManifestUri = option.storedContent.originalManifestUri;

    // If this is a stored demo asset, we'll need to configure the player with
    // license server authentication so we can delete the offline license.
    for (var i = 0; i < shakaAssets.testAssets.length; i++) {
      var originalAsset = shakaAssets.testAssets[i];
      if (originalManifestUri == originalAsset.manifestUri) {
        shakaDemo.preparePlayer_(originalAsset);
        break;
      }
    }

    p = storage.remove(offlineUri).then(function() {
      for (var i = 0; i < assetList.options.length; i++) {
        var option = assetList.options[i];
        if (option.asset && option.asset.manifestUri == originalManifestUri)
          option.isStored = false;
      }
      return shakaDemo.refreshAssetList_();
    });
  } else {
    var asset = shakaDemo.preparePlayer_(option.asset);
    var nameField = document.getElementById('offlineName').value;
    var assetName = asset.name ? '[OFFLINE] ' + asset.name : null;
    var metadata = {name: assetName || nameField || asset.manifestUri};
    p = storage.store(asset.manifestUri, metadata).then(function() {
      if (option.asset)
        option.isStored = true;
      return shakaDemo.refreshAssetList_().then(function() {
        // Auto-select offline copy of asset after storing.
        var group = shakaDemo.offlineOptGroup_;
        for (var i = 0; i < group.childNodes.length; i++) {
          var option = group.childNodes[i];
          if (option.textContent == assetName) {
            assetList.selectedIndex = option.index;
          }
        }
      });
    });
  }

  p.catch(function(reason) {
    var error = /** @type {!shaka.util.Error} */(reason);
    shakaDemo.onError_(error);
  }).then(function() {
    shakaDemo.offlineOperationInProgress_ = false;
    shakaDemo.updateButtons_(true /* canHide */);
    return storage.destroy();
  });
};


/**
 * @return {!Promise}
 * @private
 */
shakaDemo.refreshAssetList_ = function() {
  // Remove all child elements.
  var group = shakaDemo.offlineOptGroup_;
  while (group.firstChild) {
    group.removeChild(group.firstChild);
  }

  return shakaDemo.setupOfflineAssets_();
};


/**
 * @param {boolean} connected
 * @private
 */
shakaDemo.onCastStatusChange_ = function(connected) {
  if (!shakaDemo.offlineOptGroup_) {
    // No offline support.
    return;
  }

  // When we are casting, offline assets become unavailable.
  shakaDemo.offlineOptGroup_.disabled = connected;

  if (connected) {
    var assetList = document.getElementById('assetList');
    var option = assetList.options[assetList.selectedIndex];
    if (option.storedContent) {
      // This is an offline asset.  Select something else.
      assetList.selectedIndex = 0;
    }
  }
};
