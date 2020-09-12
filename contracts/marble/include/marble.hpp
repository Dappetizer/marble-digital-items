// Marble is a Digital Item format for EOSIO software.
//
// author: Craig Branscom
// company: Dappetizer, LLC
// contract: marble
// version: v0.4.0

#pragma once

#include <eosio/eosio.hpp>
#include <eosio/action.hpp>
#include <eosio/singleton.hpp>
#include <eosio/asset.hpp>

using namespace std;
using namespace eosio;

//TODO: freeze, update, release
//TODO: create release perm, linkauth to releaseall() action
//TODO: add core_symbol to config table
//TODO?: add string payload_json to trigger
//TODO?: signals

CONTRACT marble : public contract
{
    public:

    marble(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {};
    ~marble() {};

    //constants
    const symbol CORE_SYM = symbol("TLOS", 4);
    const name MINT = name("mint");
    const name TRANSFER = name("transfer");
    const name ACTIVATE = name("activate");
    const name RECLAIM = name("reclaim");
    const name CONSUME = name("consume");
    const name DESTROY = name("destroy");
    const name FREEZE = name("freeze");

    //marble core
    #include <core/config.hpp>
    #include <core/groups.hpp>
    #include <core/behaviors.hpp>
    #include <core/items.hpp>

    //marble layers
    #include <layers/tags.hpp>
    #include <layers/attributes.hpp>
    #include <layers/events.hpp>
    #include <layers/frames.hpp>
    #include <layers/bonds.hpp>

};