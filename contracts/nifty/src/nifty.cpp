#include "../include/nifty.hpp"

nifty::nifty(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {}

nifty::~nifty() {}

//======================== actions ========================

ACTION nifty::createmsg(name account_name, string message) {
    //authenticate
    require_auth(account_name);

    //open tests table, search for account name
    tests_table tests(get_self(), get_self().value);
    auto t = tests.find(account_name.value);

    //emplace new message, ram paid by account_name
    tests.emplace(account_name, [&](auto& col) {
        col.account_name = account_name;
        col.message = message;
    });
}

ACTION nifty::updatemsg(name account_name, string new_message) {
    //open tests table, search for account name
    tests_table tests(get_self(), get_self().value);
    auto& t = tests.get(account_name.value, "account not found");

    //authenticate
    require_auth(t.account_name);

    //update message
    tests.modify(t, same_payer, [&](auto& col) {
        col.message = new_message;
    });
}

ACTION nifty::deletemsg(name account_name) {
    //open tests table, search for account name
    tests_table tests(get_self(), get_self().value);
    auto& t = tests.get(account_name.value, "account not found");

    //authenticate
    require_auth(t.account_name);

    //delete message
    tests.erase(t);
}
