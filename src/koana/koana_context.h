#pragma once

#include<memory>

#include "dave/session.h"
#include "dave/decryptor.h"
#include "dave/encryptor.h"

#include "common.h"
#include "native_roster.h"
#include "native_vector.h"

using namespace dpp;
using namespace dpp::dave;
using namespace dpp::dave::mls;

using namespace mlspp;

namespace koana
{

// represents the unmanaged session, key and en/decryptors
struct koana_context
{
    // the underlying MLS session
    std::unique_ptr<session> dave_session{};

    // the transient MLS key
    std::shared_ptr<SignaturePrivateKey> mls_key;

    // the MLS decryptors, one for each user who is part of the session
    std::map<uint64_t, std::unique_ptr<decryptor>> decryptors;

    // the MLS encryptor
    std::unique_ptr<encryptor> encryptor;

    // the users and keys currently present in the session
    roster_map cached_users{};

    // logging
    void (*log)(int32_t, const char*);

    koana_context(void (*log)(int32_t, const char*))
    {
        this->log = log;
    }

    void update_cached_users(roster_map diff);

    void update_user_decryptors();
};

// -------------------------------------------------------
// first, we have the general session management functions
// -------------------------------------------------------

k_export void koana_set_mls_error_callback(void (*error_handler)(const char*, const char*));

// creates a new context with the provided logger and error handler
k_export koana_context* koana_create_context(void (*log)(int32_t, const char*));

// reinitializes the provided context. this may be called for first-time initialization too
k_export void koana_reinit_context(koana_context* context, uint16_t protocol_version, uint64_t channel_id, uint64_t bot_user_id);

// resets the given context. it cannot be used before reinitialization
k_export void koana_reset_context(koana_context* context);

// destroys the given context. it cannot be used or destroyed again
k_export void koana_destroy_context(koana_context* context);

// sets the voice gateway as the external sender for this context
k_export void koana_set_external_sender(koana_context* context, const uint8_t* data, int32_t length);

// processes a batch of incoming proposals
k_export native_vector* koana_process_proposals
(
    koana_context* context,
    const uint8_t* proposals,
    int32_t proposals_length,
    const uint64_t* known_users,
    int32_t known_users_length
);

// processes a commit from the voice gateway
k_export koana_error koana_process_commit(koana_context* context, const uint8_t* commit, int32_t commit_length);

// processes the welcome received from the voice gateway
k_export void koana_process_welcome
(
    koana_context* context, 
    const uint8_t* welcome,
    int32_t welcome_length,
    const uint64_t* known_users,
    int32_t known_users_length
);

// --------------------------------------------------------------------
// ... then in second the functions we can use to get data into managed
// --------------------------------------------------------------------

// gets the cached list of users in the MLS group
k_export native_roster* koana_get_cached_roster(koana_context* context);

// gets the current protocol version as set by the last reinitialization
k_export uint16_t koana_get_current_protocol_version(koana_context* context);

// gets the marshalled key package to send to the voice gateway.
k_export native_vector* koana_get_marshalled_key_package(koana_context* context);

// ---------------------------------------------------------
// in third, we have the decryption and encryption functions
// ---------------------------------------------------------

// decrypts a frame received from a specific user
k_export int32_t koana_decrypt_frame
(
    koana_context* context,
    uint64_t user_id,
    const uint8_t* encrypted_frame,
    int32_t encrypted_length,
    uint8_t* decrypted_frame,
    int32_t decrypted_length
);

// encrypts a frame sent by the bot
k_export koana_error koana_encrypt_frame
(
    koana_context* context,
    uint32_t ssrc,
    const uint8_t* unencrypted_frame,
    int32_t unencrypted_length,
    uint8_t* encrypted_frame,
    int32_t encrypted_length,
    int32_t* encrypted_size
);

} // namespace