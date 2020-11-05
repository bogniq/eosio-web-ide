#include <eosio/eosio.hpp>

// To add likes, consider: 
// (1) a user can't like a post for more than one time; 
// (2) a user can't like her/his own post;
// (3) unlike function, w/ similar constraints of (1) and (2).

// Message table
struct [[eosio::table("message"), eosio::contract("talk")]] message {
    uint64_t    id       = {}; // Non-0
    uint64_t    reply_to = {}; // Non-0 if this is a reply
    eosio::name user     = {};
    std::string content  = {};
    uint64_t    like_to  = {}; // Non-0 if this is a like

    uint64_t primary_key() const { return id; }
    uint64_t get_reply_to() const { return reply_to; }
    uint64_t get_like_to() const { return like_to; }
};

using message_table = eosio::multi_index<
    "message"_n, message, 
    eosio::indexed_by<"by.reply.to"_n, eosio::const_mem_fun<message, uint64_t, &message::get_reply_to>>,
    eosio::indexed_by<"by.like.to"_n, eosio::const_mem_fun<message, uint64_t, &message::get_like_to>>
    >;

// The contract
class talk : eosio::contract {
  public:
    // Use contract's constructor
    using contract::contract;

    // Post a message
    [[eosio::action]] void post(uint64_t id, uint64_t reply_to, uint64_t like_to, eosio::name user, const std::string& content) {
        message_table table{get_self(), 0};

        // Check user
        require_auth(user);

        // Check reply_to exists
        if (reply_to)
            table.get(reply_to);
        
        // Check like_to exists
        if (like_to)
            table.get(like_to);

        // Create an ID if user didn't specify one
        eosio::check(id < 1'000'000'000ull, "user-specified id is too big");
        if (!id)
            id = std::max(table.available_primary_key(), 1'000'000'000ull);

        // Record the message
        table.emplace(get_self(), [&](auto& message) {
            message.id       = id;
            message.reply_to = reply_to;
            message.user     = user;
            message.content  = content;
            message.like_to  = like_to;
        });
    }
};