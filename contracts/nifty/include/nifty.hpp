// A simple NFT standard for EOSIO software.
//
// @author Craig Branscom
// @company Dappetizer, LLC
// @contract nifty
// @version v0.0.1
// @date October 8th, 2019

#include <eosio/eosio.hpp>
#include <eosio/action.hpp>
#include <eosio/singleton.hpp>

using namespace std;
using namespace eosio;

CONTRACT nifty : public contract {

public:

    nifty(name self, name code, datastream<const char*> ds);

    ~nifty();

    //======================== actions ========================

    //create description
    ACTION createmsg(name account_name, string message);

    //update description
    ACTION updatemsg(name account_name, string new_message);

    //delete description
    ACTION deletemsg(name account_name);

    //======================== contract tables ========================

    //table description
    //scope: self
    TABLE test {
        name account_name;
        string message;

        uint64_t primary_key() const { return account_name.value; }
        EOSLIB_SERIALIZE(test, (account_name)(message))
    };
    typedef multi_index<name("tests"), test> tests_table;

};