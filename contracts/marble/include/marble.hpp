// Marble is a lightweight NFT format for EOSIO software.
//
// author: Craig Branscom
// company: Dappetizer, LLC
// contract: marble
// version: v0.3.0

#include <eosio/eosio.hpp>
#include <eosio/action.hpp>
#include <eosio/singleton.hpp>

using namespace std;
using namespace eosio;

//TODO?: add map<name, uint64_t> default_attributes
//TODO?: make blank group in init()
//TODO?: make checksum and algorithm fields optional
//TODO?: vector<name> plugins

CONTRACT marble : public contract {

    public:

    marble(name self, name code, datastream<const char*> ds);

    ~marble();

    //group settings: transferable, destructible

    //tag settings: taggable, updateable

    //attribute settings: increasable, decreasable

    //common attributes: level, power, experience, attack, defense, speed, steps, etc...

    //======================== admin actions ========================

    //initialize the contract
    ACTION init(string initial_version, name initial_admin);

    //set a new marble version
    ACTION setversion(string new_version);

    //set a new admin
    ACTION setadmin(name new_admin, string memo);

    //======================== utility actions ========================

    //log an event
    ACTION logevent(name event_name, uint64_t event_value, time_point_sec event_time, string memo);

    //======================== group actions ========================

    //creates a new nft group
    ACTION newgroup(string title, string description, name group_name, 
        name manager, uint64_t supply_cap);
    using newgroup_action = action_wrapper<"newgroup"_n, &marble::newgroup>;

    //adds a setting to a group
    ACTION addsetting(name group_name, name setting_name, bool initial_value);
    using addsetting_action = action_wrapper<"addsetting"_n, &marble::addsetting>;

    //toggles a group setting on/off
    ACTION toggle(name group_name, name setting_name, string memo);
    using toggle_action = action_wrapper<"toggle"_n, &marble::toggle>;

    //removes a setting from a group
    ACTION rmvsetting(name group_name, name setting_name);
    using rmvsetting_action = action_wrapper<"rmvsetting"_n, &marble::rmvsetting>;

    //sets a new group manager
    ACTION setmanager(name group_name, name new_manager, string memo);
    using setmanager_action = action_wrapper<"setmanager"_n, &marble::setmanager>;

    //======================== nft actions ========================

    //creates a new nft
    ACTION newnft(name owner, name group_name);
    using newnft_action = action_wrapper<"newnft"_n, &marble::newnft>;

    //transfers ownership of an nft
    ACTION transfernft(uint64_t serial, name new_owner, string memo);
    using transfernft_action = action_wrapper<"transfernft"_n, &marble::transfernft>;

    //transfers ownership of a batch of nfts
    // ACTION transfernfts(vector<uint64_t> serials, name new_owner, string memo);
    // using transfernfts_action = action_wrapper<"transfernfts"_n, &marble::transfernfts>;

    //destroys an nft
    ACTION destroynft(uint64_t serial, string memo);
    using destroynft_action = action_wrapper<"destroynft"_n, &marble::destroynft>;

    //======================== tag actions ========================

    //assign a new tag to an nft
    ACTION newtag(uint64_t serial, name tag_name, string content,
        optional<string> checksum, optional<string> algorithm);
    using newtag_action = action_wrapper<"newtag"_n, &marble::newtag>;

    //update tag content, checksum, and/or algorithm
    ACTION updatetag(uint64_t serial, name tag_name, string new_content,
        optional<string> new_checksum, optional<string> new_algorithm);
    using updatetag_action = action_wrapper<"updatetag"_n, &marble::updatetag>;

    //remove tag from nft
    ACTION removetag(uint64_t serial, name tag_name, string memo);
    using removetag_action = action_wrapper<"removetag"_n, &marble::removetag>;

    //======================== attribute actions ========================

    //assign a new attribute to an nft
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
        uint64_t last_serial;

        EOSLIB_SERIALIZE(tokenconfigs, (standard)(version)(admin)(last_serial))
    };
    typedef singleton<"tokenconfigs"_n, tokenconfigs> configs_singleton;

    //nft group data
    //scope: self
    TABLE group {
        name group_name;
        name manager;
        string title;
        string description;
        uint64_t supply;
        uint64_t issued_supply;
        uint64_t supply_cap;
        map<name, bool> settings;

        uint64_t primary_key() const { return group_name.value; }
        EOSLIB_SERIALIZE(group, 
            (group_name)(manager)(title)(description)
            (supply)(issued_supply)(supply_cap)(settings))
    };
    typedef multi_index<"groups"_n, group> groups_table;

    //individual nft data
    //scope: self
    TABLE nft {
        uint64_t serial;
        name group;
        name owner;

        uint64_t primary_key() const { return serial; }
        uint64_t by_group() const { return group.value; }
        uint64_t by_owner() const { return owner.value; }
        EOSLIB_SERIALIZE(nft, (serial)(group)(owner))
    };
    typedef multi_index<"nfts"_n, nft,
        indexed_by<"bygroup"_n, const_mem_fun<nft, uint64_t, &nft::by_group>>,
        indexed_by<"byowner"_n, const_mem_fun<nft, uint64_t, &nft::by_owner>>
    > nfts_table;

    //tag content
    //scope: serial
    TABLE tag {
        name tag_name;
        string content;
        string checksum;
        string algorithm;

        uint64_t primary_key() const { return tag_name.value; }
        EOSLIB_SERIALIZE(tag, (tag_name)(content)(checksum)(algorithm))
    };
    typedef multi_index<"tags"_n, tag> tags_table;

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