// A lightweight NFT standard for EOSIO software.
//
// @author Craig Branscom
// @company Dappetizer, LLC
// @contract nifty
// @version v0.2.0
// @date October 15th, 2019

#include <eosio/eosio.hpp>
#include <eosio/action.hpp>
#include <eosio/singleton.hpp>

using namespace std;
using namespace eosio;

//TODO?: add map<name, uint64_t> default_attributes
//TODO?: make blank set in init()
//TODO?: make checksum and algorithm fields optional

CONTRACT nifty : public contract {

    public:

    nifty(name self, name code, datastream<const char*> ds);

    ~nifty();

    //collection options: transferable, destructible, updateable, increasable, decreasable

    //common attributes: level, power, experience, attack, defense, speed, steps, etc...

    //======================== admin actions ========================

    //initialize the contract
    ACTION init(string initial_version, name initial_access);

    //sets a new nifty version
    ACTION setversion(string new_version);

    //sets a new admin
    ACTION setadmin(name new_admin, string memo);

    //sets a new access method
    ACTION setaccess(name new_access, string memo);

    //======================== collection actions ========================

    //creates a new nft collection
    ACTION newcollectn(string title, string description, name collection_name, 
        name manager, uint64_t supply_cap);

    //adds an nft option
    ACTION addoption(name collection_name, name option_name, bool initial_value);

    //toggles a collection option on/off
    ACTION toggle(name collection_name, name option_name, string memo);

    //removes an nft option
    ACTION rmvoption(name collection_name, name option_name);

    //sets a new manager
    ACTION setmanager(name collection_name, name new_manager, string memo);

    //======================== nft actions ========================

    //creates a new nft
    ACTION newnft(name owner, name collection_name, string content, 
        optional<string> checksum, optional<string> algorithm);

    //updates nft content, checksum, and algorithm
    ACTION updatenft(uint64_t serial, string content, 
        optional<string> checksum, optional<string> algorithm);

    //transfers ownership of an nft
    ACTION transfernft(uint64_t serial, name new_owner, string memo);

    //destroys an nft
    ACTION destroynft(uint64_t serial, string memo);

    //======================== attribute actions ========================

    //adds a new attribute to an nft
    ACTION newattribute(uint64_t serial, name attribute_name, uint64_t initial_points);

    //sets an attributes points
    ACTION setpoints(uint64_t serial, name attribute_name, uint64_t new_points);

    //increases attribute points by amount
    ACTION increasepts(uint64_t serial, name attribute_name, uint64_t points_to_add);

    //decreases attribute points by amount
    ACTION decreasepts(uint64_t serial, name attribute_name, uint64_t points_to_subtract);

    //removes an attribute from an nft
    ACTION rmvattribute(uint64_t serial, name attribute_name);

    //======================== contract tables ========================

    //contract configs
    //scope: singleton
    TABLE tokenconfigs {
        name standard;
        string version;
        name admin;
        name access;
        uint64_t last_serial;

        EOSLIB_SERIALIZE(tokenconfigs, 
            (standard)(version)(admin)(access)(last_serial))
    };
    typedef singleton<"tokenconfigs"_n, tokenconfigs> configs_singleton;

    //nft collection data
    //scope: self
    TABLE collection {
        name collection_name;
        name manager;
        string title;
        string description;
        uint64_t supply;
        uint64_t issued_supply;
        uint64_t supply_cap;
        map<name, bool> options;

        uint64_t primary_key() const { return collection_name.value; }
        EOSLIB_SERIALIZE(collection, 
            (collection_name)(manager)(title)(description)
            (supply)(issued_supply)(supply_cap)(options))
    };
    typedef multi_index<"collections"_n, collection> collections_table;

    //individual nft data
    //scope: self
    TABLE nft {
        uint64_t serial;
        name collection;
        name owner;

        string content; //json, markdown, dStor/IPFS cid
        string checksum; //checksum of content
        string algorithm; //algorithm used to produce checksum

        uint64_t primary_key() const { return serial; }
        uint64_t by_collection() const { return collection.value; }
        uint64_t by_owner() const { return owner.value; }
        EOSLIB_SERIALIZE(nft,
            (serial)(collection)(owner)
            (content)(checksum)(algorithm))
    };
    typedef multi_index<"nfts"_n, nft,
        indexed_by<"bycollection"_n, const_mem_fun<nft, uint64_t, &nft::by_collection>>,
        indexed_by<"byowner"_n, const_mem_fun<nft, uint64_t, &nft::by_owner>>
    > nfts_table;

    //attributes data
    //scope: serial
    TABLE attribute {
        name attribute_name;
        uint64_t points;

        uint64_t primary_key() const { return attribute_name.value; }
        EOSLIB_SERIALIZE(attribute, (attribute_name)(points))
    };
    typedef multi_index<name("attributes"), attribute> attributes_table;

};