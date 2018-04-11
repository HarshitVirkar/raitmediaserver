// Copyright 2014 Google Inc. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PACKAGER_MEDIA_BASE_WIDEVINE_KEY_SOURCE_H_
#define PACKAGER_MEDIA_BASE_WIDEVINE_KEY_SOURCE_H_

#include <map>
#include <memory>
#include "packager/base/synchronization/waitable_event.h"
#include "packager/base/values.h"
#include "packager/media/base/closure_thread.h"
#include "packager/media/base/fourccs.h"
#include "packager/media/base/key_source.h"

namespace shaka {
namespace media {

const uint8_t kWidevineSystemId[] = {0xed, 0xef, 0x8b, 0xa9, 0x79, 0xd6,
                                     0x4a, 0xce, 0xa3, 0xc8, 0x27, 0xdc,
                                     0xd5, 0x1d, 0x21, 0xed};

class KeyFetcher;
class RequestSigner;
template <class T> class ProducerConsumerQueue;

/// WidevineKeySource talks to the Widevine encryption service to
/// acquire the encryption keys.
class WidevineKeySource : public KeySource {
 public:
  /// @param server_url is the Widevine common encryption server url.
  WidevineKeySource(const std::string& server_url, bool add_common_pssh);

  ~WidevineKeySource() override;

  /// @name KeySource implementation overrides.
  /// @{
  Status FetchKeys(EmeInitDataType init_data_type,
                   const std::vector<uint8_t>& init_data) override;
  Status GetKey(const std::string& stream_label, EncryptionKey* key) override;
  Status GetKey(const std::vector<uint8_t>& key_id,
                EncryptionKey* key) override;
  Status GetCryptoPeriodKey(uint32_t crypto_period_index,
                            const std::string& stream_label,
                            EncryptionKey* key) override;
  /// @}

  /// Fetch keys for CENC from the key server.
  /// @param content_id the unique id identify the content.
  /// @param policy specifies the DRM content rights.
  /// @return OK on success, an error status otherwise.
  Status FetchKeys(const std::vector<uint8_t>& content_id,
                   const std::string& policy);

  /// Set the protection scheme for the key source.
  void set_protection_scheme(FourCC protection_scheme) {
    protection_scheme_ = protection_scheme;
  }

  /// Set signer for the key source.
  /// @param signer signs the request message.
  void set_signer(std::unique_ptr<RequestSigner> signer);

  /// Inject an @b KeyFetcher object, mainly used for testing.
  /// @param key_fetcher points to the @b KeyFetcher object to be injected.
  void set_key_fetcher(std::unique_ptr<KeyFetcher> key_fetcher);

  // Set the group id for the key source
  // @param group_id group identifier
  void set_group_id(const std::vector<uint8_t>& group_id);

 private:
  typedef std::map<std::string, std::unique_ptr<EncryptionKey>>
      EncryptionKeyMap;
  typedef ProducerConsumerQueue<std::shared_ptr<EncryptionKeyMap>>
      EncryptionKeyQueue;

  // Internal routine for getting keys.
  Status GetKeyInternal(uint32_t crypto_period_index,
                        const std::string& stream_label,
                        EncryptionKey* key);

  // The closure task to fetch keys repeatedly.
  void FetchKeysTask();

  // Fetch keys from server.
  Status FetchKeysInternal(bool enable_key_rotation,
                           uint32_t first_crypto_period_index,
                           bool widevine_classic);

  // Fill |request| with necessary fields for Widevine encryption request.
  // |request| should not be NULL.
  void FillRequest(bool enable_key_rotation,
                   uint32_t first_crypto_period_index,
                   std::string* request);
  // Base64 escape and format the request. Optionally sign the request if a
  // signer is provided. |message| should not be NULL. Return OK on success.
  Status GenerateKeyMessage(const std::string& request, std::string* message);
  // Decode |response| from JSON formatted |raw_response|.
  // |response| should not be NULL.
  bool DecodeResponse(const std::string& raw_response, std::string* response);
  // Extract encryption key from |response|, which is expected to be properly
  // formatted. |transient_error| will be set to true if it fails and the
  // failure is because of a transient error from the server. |transient_error|
  // should not be NULL.
  bool ExtractEncryptionKey(bool enable_key_rotation,
                            bool widevine_classic,
                            const std::string& response,
                            bool* transient_error);
  // Push the keys to the key pool.
  bool PushToKeyPool(EncryptionKeyMap* encryption_key_map);

  ClosureThread key_production_thread_;
  // The fetcher object used to fetch keys from the license service.
  // It is initialized to a default fetcher on class initialization.
  // Can be overridden using set_key_fetcher for testing or other purposes.
  std::unique_ptr<KeyFetcher> key_fetcher_;
  std::string server_url_;
  std::unique_ptr<RequestSigner> signer_;
  base::DictionaryValue request_dict_;

  const uint32_t crypto_period_count_;
  FourCC protection_scheme_;
  base::Lock lock_;
  bool add_common_pssh_;
  bool key_production_started_;
  base::WaitableEvent start_key_production_;
  uint32_t first_crypto_period_index_;
  std::vector<uint8_t> group_id_;
  std::unique_ptr<EncryptionKeyQueue> key_pool_;
  EncryptionKeyMap encryption_key_map_;  // For non key rotation request.
  Status common_encryption_request_status_;

  DISALLOW_COPY_AND_ASSIGN(WidevineKeySource);
};

}  // namespace media
}  // namespace shaka

#endif  // PACKAGER_MEDIA_BASE_WIDEVINE_KEY_SOURCE_H_
