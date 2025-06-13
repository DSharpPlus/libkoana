#include "koana_context.h"

#include "dave/log_level.h"

using namespace dpp;
using namespace dpp::dave;
using namespace dpp::dave::mls;

using namespace mlspp;

namespace koana
{

void (*mls_error_logger)(const char*, const char*);

void mls_error_callback(const std::string& source, const std::string& reason)
{
    const char* c_source = source.c_str();
    const char* c_reason = reason.c_str();

    mls_error_logger(c_source, c_reason);
}

k_export void koana_set_mls_error_callback(void (*error_handler)(const char*, const char*))
{
    mls_error_logger = error_handler;
}

k_export koana_context* koana_create_context(void (*log)(int32_t, const char*))
{
    std::function<void(const std::string&, const std::string&)> error_callback = mls_error_callback;

    koana_context* context = new koana_context(log);
    context->dave_session = std::make_unique<session>(log, nullptr, 0ull, error_callback);
    context->bot_encryptor = std::make_unique<encryptor>(log);

    return context;
}

k_export void koana_reinit_context(koana_context* context, uint16_t protocol_version, uint64_t channel_id, uint64_t bot_user_id)
{
    context->bot_id = bot_user_id;
    context->dave_session->init(protocol_version, channel_id, bot_user_id, context->mls_key);
}

k_export void koana_reset_context(koana_context* context)
{
    context->dave_session->reset();
    context->decryptors.clear();
    context->cached_users.clear();
}

k_export void koana_destroy_context(koana_context* context)
{
    delete context;
}

k_export void koana_set_external_sender(koana_context* context, const uint8_t* data, int32_t length)
{
    std::vector<uint8_t> vec(data, data + length);
    context->dave_session->set_external_sender(vec);
}

k_export native_vector* koana_process_proposals
(
    koana_context* context,
    const uint8_t* proposals,
    int32_t proposals_length,
    const uint64_t* known_users,
    int32_t known_users_length
)
{
    std::vector<uint8_t> vec(proposals, proposals + proposals_length);
    std::set<uint64_t> users {};

    for (int32_t i = 0; i < known_users_length; ++i)
    {
        users.insert(known_users[i]);
    }

    std::optional<std::vector<uint8_t>> result = context->dave_session->process_proposals(vec, users);

    if (result.has_value())
    {
        return new native_vector(result.value());
    }
    else
    {
        return nullptr;
    }
}

k_export koana_error koana_process_commit(koana_context* context, const uint8_t* commit, int32_t commit_length)
{
    std::vector<uint8_t> vec(commit, commit + commit_length);
    
    roster_variant result = context->dave_session->process_commit(vec);

    if (std::holds_alternative<failed_t>(result))
    {
        return mls_commit_reset_error;
    }
    else if (std::holds_alternative<ignored_t>(result))
    {
        return mls_commit_ignorable_error;
    }
    else
    {
        roster_map roster = std::get<roster_map>(result);
        context->update_cached_users(roster);
        return success;
    }
}

k_export void koana_process_welcome
(
    koana_context* context, 
    const uint8_t* welcome,
    int32_t welcome_length,
    const uint64_t* known_users,
    int32_t known_users_length
)
{
    std::vector<uint8_t> vec(welcome, welcome + welcome_length);
    std::set<uint64_t> users {};

    for (int32_t i = 0; i < known_users_length; ++i)
    {
        users.insert(known_users[i]);
    }

    std::optional<roster_map> roster = context->dave_session->process_welcome(vec, users);
    
    if (roster.has_value())
    {
        context->cached_users = roster.value();
        context->update_user_decryptors();
    }
}

k_export native_roster* koana_get_cached_roster(koana_context* context)
{
    return new native_roster(context->cached_users);
}

k_export uint16_t koana_get_current_protocol_version(koana_context* context)
{
    return context->dave_session->get_protocol_version();
}

k_export native_vector* koana_get_marshalled_key_package(koana_context* context)
{
    return new native_vector(context->dave_session->get_marshalled_key_package());
}

k_export int32_t koana_decrypt_frame
(
    koana_context* context,
    uint64_t user_id,
    const uint8_t* encrypted_frame,
    int32_t encrypted_length,
    uint8_t* decrypted_frame,
    int32_t decrypted_length
)
{
    array_view<uint8_t const> encrypted = array_view<uint8_t const>(encrypted_frame, (size_t)encrypted_length);
    array_view<uint8_t> decrypted = array_view<uint8_t>(decrypted_frame, (size_t)decrypted_length);

    size_t decrypted_size = context->decryptors[user_id]->decrypt(media_audio, encrypted, decrypted);

    // having to decrypt a 2gb+ packet in a real-time gateway would be a little unhinged.
    return (int32_t)decrypted_size;
}

k_export koana_error koana_encrypt_frame
(
    koana_context* context,
    uint32_t ssrc,
    const uint8_t* unencrypted_frame,
    int32_t unencrypted_length,
    uint8_t* encrypted_frame,
    int32_t encrypted_length,
    int32_t* encrypted_size
)
{
    array_view<uint8_t const> unencrypted = array_view<uint8_t const>(unencrypted_frame, (size_t)unencrypted_length);
    array_view<uint8_t> encrypted = array_view<uint8_t>(encrypted_frame, (size_t)encrypted_length);

    size_t size;
    encryptor::result_code result = context->bot_encryptor->encrypt(media_audio, ssrc, unencrypted, encrypted, &size);

    if (result == encryptor::rc_encryption_failure)
    {
        return encryption_failure;
    }

    *encrypted_size = (int32_t)size;
    return success;
}

// context helper implementation

void koana_context::update_cached_users(roster_map diff)
{
    if (this->cached_users.empty())
    {
        this->cached_users = diff;
        return;
    }

    this->log(ll_debug, "updating cached mls roster");

    for (const auto&[id, key] : diff)
    {
        if (key.empty())
        {
            this->cached_users.erase(id);
            this->decryptors.erase(id);

            this->log(ll_debug, ("removed user " + std::to_string(id) + " from MLS group").c_str());

            continue;
        }

        roster_map::iterator cached_key = this->cached_users.find(id);

        this->cached_users[id] = key;

        if (cached_key == this->cached_users.end())
        {
            this->log(ll_debug, ("added user " + std::to_string(id) + " to MLS group").c_str());
        }
        else
        {
            this->log(ll_debug, ("updated mls key for user " + std::to_string(id)).c_str());
        }
    }

    this->update_user_decryptors();
}

void koana_context::update_user_decryptors()
{
    if (this->cached_users.empty())
    {
        this->log(ll_error, "no users were in the call when trying to update decryptors for users!");
        return;
    }

    this->log(ll_debug, ("updating decryptors for " + std::to_string(this->cached_users.size()) + " users").c_str());

    for (const auto&[id, key] : this->cached_users)
    {
        if (id == this->bot_id)
        {
            continue;
        }

        std::map<uint64_t, std::unique_ptr<dpp::dave::decryptor>>::iterator current_decryptor = this->decryptors.find(id);

        if (current_decryptor == this->decryptors.end())
        {
            this->log(ll_debug, ("creating new decryptor for user " + std::to_string(id)).c_str());
            auto [iter, inserted] = this->decryptors.emplace(id, std::make_unique<decryptor>(this->log));
            current_decryptor = iter;
        }

        current_decryptor->second->transition_to_key_ratchet(this->dave_session->get_key_ratchet(id));
    }

    this->bot_encryptor->set_key_ratchet(this->dave_session->get_key_ratchet(this->bot_id));
}

} // namespace