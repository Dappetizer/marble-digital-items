// Marble is a lightweight NFT standard for EOSIO software.
//
// author: Craig Branscom
// company: Dappetizer, LLC
// contract: marble
// version: v0.2.0
// created: October 15th, 2019

#include <eosio/eosio.hpp>
#include <eosio/action.hpp>
#include <eosio/singleton.hpp>

using namespace std;
using namespace eosio;

//TODO?: add map<name, uint64_t> default_attributes
//TODO?: make blank set in init()
//TODO?: make checksum and algorithm fields optional

CONTRACT marble : public contract {

    public:

    marble(name self, name code, datastream<const char*> ds);

    ~marble();

    //collection options: transferable, destructible, updateable, increasable, decreasable

    //common attributes: level, power, experience, attack, defense, speed, steps, etc...

    //======================== admin actions ========================

    //initialize the contract
    ACTION init(string initial_version, name initial_access);

    //sets a new marble version
    ACTION setversion(string new_version);

    //sets a new admin
    ACTION setadmin(name new_admin, string memo);

    //sets a new access method
    ACTION setaccess(name new_access, string memo);

    //======================== collection actions ========================

    //creates a new nft collection
    ACTION newcollectn(string title, string description, name collection_name, 
        name manager, uint64_t supply_cap);
    using newcollectn_action = action_wrapper<"newcollectn"_n, &marble::newcollectn>;

    //adds an nft option
    ACTION addoption(name collection_name, name option_name, bool initial_value);
    using addoption_action = action_wrapper<"addoption"_n, &marble::addoption>;

    //toggles a collection option on/off
    ACTION toggle(name collection_name, name option_name, string memo);
    using toggle_action = action_wrapper<"toggle"_n, &marble::toggle>;

    //removes an nft option
    ACTION rmvoption(name collection_name, name option_name);
    using rmvoption_action = action_wrapper<"rmvoption"_n, &marble::rmvoption>;

    //sets a new manager
    ACTION setmanager(name collection_name, name new_manager, string memo);
    using setmanager_action = action_wrapper<"setmanager"_n, &marble::setmanager>;

    //======================== nft actions ========================

    //creates a new nft
    ACTION newnft(name owner, name collection_name, string content, 
        optional<string> checksum, optional<string> algorithm);
    using newnft_action = action_wrapper<"newnft"_n, &marble::newnft>;

    //updates nft content, checksum, and algorithm
    ACTION updatenft(uint64_t serial, string content, 
        optional<string> checksum, optional<string> algorithm);
    using updatenft_action = action_wrapper<"updatenft"_n, &marble::updatenft>;

    //transfers ownership of an nft
    ACTION transfernft(uint64_t serial, name new_owner, string memo);
    using transfernft_action = action_wrapper<"transfernft"_n, &marble::transfernft>;

    //destroys an nft
    ACTION destroynft(uint64_t serial, string memo);
    using destroynft_action = action_wrapper<"destroynft"_n, &marble::destroynft>;

    //======================== attribute actions ========================

    //adds a new attribute to an nft
    ACTION newattribute(uint64_t serial, name attribute_name, uint64_t initial_points);
    using newattribute_action = action_wrapper<"newattribute"_n, &marble::newattribute>;

    //sets an attributes points
    ACTION setpoints(uint64_t serial, name attribute_name, uint64_t new_points);
    using setpoints_action = action_wrapper<"setpoints"_n, &marble::setpoints>;

    //increases attribute points by amount
    ACTION increasepts(uint64_t serial, name attribute_name, uint64_t points_to_add);
    using increasepts_action = action_wrapper<"increasepts"_n, &marble::increasepts>;

    //decreases attribute points by amount
    ACTION decreasepts(uint64_t serial, name attribute_name, uint64_t points_to_subtract);
    using decreasepts_action = action_wrapper<"decreasepts"_n, &marble::decreasepts>;

    //removes an attribute from an nft
    ACTION rmvattribute(uint64_t serial, name attribute_name);
    using rmvattribute_action = action_wrapper<"rmvattribute"_n, &marble::rmvattribute>;

    //======================== contract tables ========================

    //contract configs
    //scope: singleton
    TABLE tokenconfigs {
        name standard; //"marble"_n
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