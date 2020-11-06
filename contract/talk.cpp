#include <eosio/eosio.hpp>

// Message table
struct [[eosio::table("message"), eosio::contract("talk")]] message {
    uint64_t    id       = {}; // Non-0
    uint64_t    reply_to = {}; // Non-0 if this is a reply
    eosio::name user     = {};
    std::string content  = {};

    uint64_t primary_key() const { return id; }
    uint64_t get_reply_to() const { return reply_to; }
};

using message_table = eosio::multi_index<"message"_n, message, 
    eosio::indexed_by<"by.reply.to"_n, eosio::const_mem_fun<message, uint64_t, &message::get_reply_to>>>;

struct [[eosio::table("msglikes"), eosio::contract("talk")]] msglikes {
    uint64_t    id      = {}; // Non-0
    uint64_t    msgId   = {}; // Non-0
    eosio::name user    = {};

    uint64_t primary_key() const { return id; }
    uint64_t get_msgId() const { return msgId; }
};

using likes_table = eosio::multi_index<"msglikes"_n, msglikes, 
    eosio::indexed_by<"by.msgid"_n, eosio::const_mem_fun<msglikes, uint64_t, &msglikes::get_msgId>>>;

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
        //message_table table{get_self(), 0};

        // Check user
        require_auth(user);

        // Check reply_to exists
        if (reply_to)
            msgs.get(reply_to);
            //table.get(reply_to);

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

    // Like a message
    // (1) a user can't like a post for more than one time; 
    // (2) a user can't like her/his own post;
    [[eosio::action]] void like(uint64_t id, uint64_t msgId, eosio::name user) {
        // Check user
        require_auth(user);        

        eosio::check(id < 1'000'000'000ull, "user-specified id is too big");
        if (!id)
            id = std::max(likes.available_primary_key(), 1'000'000'000ull);
        
        // Check msgId exists
        auto itr = msgs.find(msgId);
        eosio::check( itr != msgs.end(), "message does not exist in table" );

        // Check msgId was not posted by user
        eosio::check( itr->user != user, "message was posted by this user her/himself");

        // Check user hasn't liked msgId
        for (auto& item : likes) {
            eosio::check( item.user != user, "this user has already liked the message" ); // TODO: is it good to use `check()` within a loop
        }

        // Record the message
        likes.emplace(get_self(), [&](auto& msglikes) {
            msglikes.id     = id;
            msglikes.user   = user;
            msglikes.msgId  = msgId;
        });
    }

    // Unlike a message
    [[eosio::action]] void unlike(uint64_t id, uint64_t msgId, eosio::name user) {
        // Check user
        require_auth(user);        

        eosio::check(id < 1'000'000'000ull, "user-specified id is too big");
        if (!id)
            id = std::max(likes.available_primary_key(), 1'000'000'000ull);

        // Check user has liked msgId
        bool liked = false;
        for (auto& item : likes) {
            if (item.msgId == msgId && item.user == user) {
                liked = true;
                likes.erase(item);
                break;
            }
        }
        eosio::check(liked, "this message doesn't exist, OR this user hasn't liked the message");
    }
};