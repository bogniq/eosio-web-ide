#include <boost/test/unit_test.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/testing/tester.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

BOOST_AUTO_TEST_SUITE(talk_tests)

BOOST_AUTO_TEST_CASE(post_and_like) try {
    tester t{setup_policy::none};

    // Load contract
    t.create_account(N(talk));
    t.set_code(N(talk), read_wasm("talk.wasm"));
    t.set_abi(N(talk), read_abi("talk.abi").data());
    t.produce_block();

    // Create users
    t.create_account(N(john));
    t.create_account(N(jane));
    t.create_account(N(bob));

    // Test "post" action
    t.push_action(
        N(talk), N(post), N(john),
        mutable_variant_object //
        ("id", 1)              //
        ("reply_to", 0)        //
        ("user", "john")       //
        ("content", "post 1")  //
    );

    // t.produce_block();
    // t.push_action(
    //     N(talk), N(post), N(john),
    //     mutable_variant_object //
    //     ("id", 1)              //
    //     ("reply_to", 0)        //
    //     ("user", "john")       //
    //     ("content", "post 1")  //
    // );

    t.push_action(
        N(talk), N(post), N(jane),
        mutable_variant_object //
        ("id", 2)              //
        ("reply_to", 0)        //
        ("user", "jane")       //
        ("content", "post 2")  //
    );
    t.push_action(
        N(talk), N(post), N(john),
        mutable_variant_object       //
        ("id", 3)                    //
        ("reply_to", 2)              //
        ("user", "john")             //
        ("content", "post 3: reply") //
    );

    //Test "like" action
    t.push_action(
        N(talk), N(like), N(jane),
        mutable_variant_object //
        ("id", 0)              //
        ("msgId", 1)        //
        ("user", "jane")       //
    );

    //Check likes num is 1
    t.push_action(
        N(talk), N(checklikes), N(jane),
        mutable_variant_object //
        ("msgId", 1)        //
        ("num", 1)       //
    );

    //Another "like" action
    t.push_action(
        N(talk), N(like), N(bob),
        mutable_variant_object //
        ("id", 0)              //
        ("msgId", 1)        //
        ("user", "bob")       //
    );

    //Check likes num is 2
    t.push_action(
        N(talk), N(checklikes), N(jane),
        mutable_variant_object //
        ("msgId", 1)        //
        ("num", 2)       //
    );

    // Unlike
    t.push_action(
        N(talk), N(like), N(jane),
        mutable_variant_object //
        ("id", 1)              // id can not be 0; otherwise Duplicate transaction error
        ("msgId", 1)        //
        ("user", "jane")       //
    );

    //Check likes num is 1
    t.push_action(
        N(talk), N(checklikes), N(bob), // user can not be jane; otherwise Duplicate transaction error
        mutable_variant_object //
        ("msgId", 1)        //
        ("num", 1)       //
    );

    // Can't reply to non-existing message
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                N(talk), N(post), N(john),
                mutable_variant_object       //
                ("id", 4)                    //
                ("reply_to", 99)             //
                ("user", "john")             //
                ("content", "post 3: reply") //
            );
        }(),
        fc::exception);

    // Can't like as another user
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                N(talk), N(like), N(jane),
                mutable_variant_object //
                ("id", 1)              //
                ("msgId", 1)        //
                ("user", "john")       //
            );
        }(),
        fc::exception);

    // Can't like non-existing message
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                N(talk), N(like), N(jane),
                mutable_variant_object //
                ("id", 1)              //
                ("msgId", 99)        //
                ("user", "jane")       //
            );
        }(),
        fc::exception);

    // Can't like her/his own post
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                N(talk), N(like), N(jane),
                mutable_variant_object //
                ("id", 1)              //
                ("msgId", 1)        //
                ("user", "jane")       //
            );
        }(),
        fc::exception);
}
FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
