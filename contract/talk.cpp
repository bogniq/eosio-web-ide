#include <eosio/eosio.hpp>

// Message table
struct [[eosio::table("message"), eosio::contract("talk")]] message {
    uint64_t    id          = {}; // Non-0
    uint64_t    reply_to    = {}; // Non-0 if this is a reply
    eosio::name user        = {};
    std::string content     = {};
    uint64_t    likes_num   = {};

    uint64_t primary_key() const { return id; }
    uint64_t get_reply_to() const { return reply_to; }
};

using message_table = eosio::multi_index<"message"_n, message, 
    eosio::indexed_by<"by.reply.to"_n, eosio::const_mem_fun<message, uint64_t, &message::get_reply_to>>>;

static uint128_t create_key(uint64_t msgId, const eosio::name &user) {
    return uint128_t{msgId} << 64 | user.value;
}

struct [[eosio::table("msglikes"), eosio::contract("talk")]] msglikes {
    uint64_t    id      = {}; // Non-0
    uint64_t    msgId   = {}; // Non-0
    eosio::name user    = {};

    uint64_t primary_key() const { return id; }
    uint128_t secondary_key() const { return create_key(msgId, user); }
};

using likes_table = eosio::multi_index<"msglikes"_n, msglikes, 
    eosio::indexed_by<"by.secondary"_n, eosio::const_mem_fun<msglikes, uint128_t, &msglikes::secondary_key>>>;

// The contract
class talk : eosio::contract {
  public:
    message_table msgs;
    likes_table likes;

    // Use contract's constructor
    using contract::contract;
    talk(eosio::name receiver, eosio::name code, eosio::datastream<const char*> ds)
        : contract(receiver, code, ds), msgs(_self, 0), likes(_self, 0) {}

    // Post a message
    [[eosio::action]] void post(uint64_t id, uint64_t reply_to, eosio::name user, const std::string& content) {
        // Check user
        require_auth(user);

        // Check reply_to exists
        if (reply_to)
            msgs.get(reply_to);

        // Create an ID if user didn't specify one
        eosio::check(id < 1'000'000'000ull, "user-specified id is too big");
        if (!id)
            id = std::max(msgs.available_primary_key(), 1'000'000'000ull);

        // Record the message
        msgs.emplace(get_self(), [&](auto& message) {
        //table.emplace(get_self(), [&](auto& message) {
            message.id       = id;
            message.reply_to = reply_to;
            message.user     = user;
            message.content  = content;
        });
    }

    // Like & Unlike a message
    [[eosio::action]] void like(uint64_t id, uint64_t msgId, eosio::name user) {
        // Check user
        require_auth(user);        

        eosio::check(id < 1'000'000'000ull, "user-specified id is too big");
        if (!id)
            id = std::max(likes.available_primary_key(), 1'000'000'000ull);
        
        // Check msgId exists
        auto it_msgs = msgs.find(msgId);
        eosio::check( it_msgs != msgs.end(), "message does not exist in table" );

        // Check msgId was not posted by user
        eosio::check( it_msgs->user != user, "message can't be liked by its poster");

        // If a user has already liked a post, liking it again is treated as unlike
        auto idx = likes.get_index<"by.secondary"_n>();
        auto it_likes = idx.find(create_key(msgId, user));

        if (it_likes != idx.end()) {
            idx.erase(it_likes);
            msgs.modify(it_msgs, user, [&](auto& message) {
                message.likes_num--;
            });
        } else {
            // Record the message
            likes.emplace(get_self(), [&](auto& msglikes) {
                msglikes.id     = id;
                msglikes.user   = user;
                msglikes.msgId  = msgId;
            });
            msgs.modify(it_msgs, user, [&](auto& message) {
                message.likes_num++;
            });
        }
    }
};