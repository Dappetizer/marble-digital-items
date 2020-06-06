// Marble is a modular digital item format for EOSIO software.
//
// author: Craig Branscom
// company: Dappetizer, LLC
// contract: marble
// version: v1.0.0

#include <eosio/eosio.hpp>
#include <eosio/action.hpp>
#include <eosio/singleton.hpp>

using namespace std;
using namespace eosio;

//TODO?: make checksum and algorithm fields optional
//TODO?: allow frame default overrides (map overrides over defaults, then apply frame)
//TODO?: group locking/unlocking + unlock_acct, unlock_auth
//TODO?: cleanframe() action to clean an item frame from an item

CONTRACT marble : public contract {

    public:

    marble(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {};
    ~marble() {};

    //behaviors: mint, transfer, activate, consume, destroy
    //TODO: reclaim, freeze, update

    //======================== admin actions ========================

    //initialize the contract
    //auth: self
    ACTION init(string contract_name, string contract_version, name initial_admin);

    //set new contract version
    //auth: admin
    ACTION setversion(string new_version);

    //set new admin
    //auth: admin
    ACTION setadmin(name new_admin);

    //======================== group actions ========================

    //create a new item group
    //auth: admin
    ACTION newgroup(string title, string description, name group_name, name manager, uint64_t supply_cap);

    //edit group title and description
    //auth: manager
    ACTION editgroup(name group_name, string new_title, string new_description);

    //set a new group manager
    //auth: manager
    ACTION setmanager(name group_name, name new_manager, string memo);

    //======================== behavior actions ========================

    //add a behavior to a group
    //auth: manager
    ACTION addbehavior(name group_name, name behavior_name, bool initial_state);

    //toggle a behavior on/off
    //auth: manager
    ACTION toggle(name group_name, name behavior_name);

    //remove a behavior from a group
    //auth: manager
    ACTION rmvbehavior(name group_name, name behavior_name);

    //======================== item actions ========================

    //mint a new item
    //auth: manager
    ACTION mintitem(name to, name group_name);

    //transfer ownership of one or more items
    //auth: owner
    ACTION transferitem(name from, name to, vector<uint64_t> serials, string memo);

    //activate an item
    //auth: owner
    ACTION activateitem(uint64_t serial);

    //consume an item
    //auth: owner
    ACTION consumeitem(uint64_t serial);

    //reclaim an item
    //auth: manager
    // ACTION reclaimitem(uint64_t serial);

    //freeze an item
    //auth: manager
    // ACTION freezeitem(uint64_t serial);

    //destroy an item
    //auth: manager
    ACTION destroyitem(uint64_t serial, string memo);

    //======================== tag actions ========================

    //assign a new tag to an item
    //auth: manager
    ACTION newtag(uint64_t serial, name tag_name, string content, optional<string> checksum, optional<string> algorithm);

    //update tag content, checksum, and/or algorithm
    //auth: manager
    ACTION updatetag(uint64_t serial, name tag_name, string new_content, optional<string> new_checksum, optional<string> new_algorithm);

    //remove tag from item
    //auth: manager
    ACTION rmvtag(uint64_t serial, name tag_name, string memo);

    //======================== attribute actions ========================

    //assign a new attribute to an item
    //auth: manager
    ACTION newattribute(uint64_t serial, name attribute_name, int64_t initial_points);

    //sets an attributes points
    //auth: manager
    ACTION setpoints(uint64_t serial, name attribute_name, int64_t new_points);

    //increases attribute points by amount
    //auth: manager
    ACTION increasepts(uint64_t serial, name attribute_name, uint64_t points_to_add);

    //decreases attribute points by amount
    //auth: manager
    ACTION decreasepts(uint64_t serial, name attribute_name, uint64_t points_to_subtract);

    //removes an attribute from an item
    //auth: manager
    ACTION rmvattribute(uint64_t serial, name attribute_name);

    //======================== event actions ========================

    //log an event (will not save to events table)
    //auth: self
    ACTION logevent(name event_name, int64_t event_value, time_point_sec event_time, string memo);

    //create a new event (if no custom event time given use current time point)
    //auth: manager
    ACTION newevent(uint64_t serial, name event_name, optional<time_point_sec> custom_event_time);

    //set a custom time on an event
    //auth: manager
    ACTION seteventtime(uint64_t serial, name event_name, time_point_sec new_event_time);

    //remove an event
    //auth: manager
    ACTION rmvevent(uint64_t serial, name event_name);

    //======================== frame actions ========================

    //set up a new frame
    //auth: manager
    ACTION newframe(name frame_name, name group, map<name, string> default_tags, map<name, int64_t> default_attributes);

    //applies a frame to an item
    //auth: manager
    ACTION applyframe(name frame_name, uint64_t serial, bool overwrite);

    //mints a new item and applies a frame immediately with tag and attribute default overrides
    //auth: manager
    ACTION quickbuild(name frame_name, name to, map<name, string> override_tags, map<name, int64_t> override_attributes);

    //remove a frame
    //auth: manager
    ACTION rmvframe(name frame_name, string memo);

    //======================== contract tables ========================

    //tokenconfigs for tracking in @cc32d9's repo
    //scope: self
    // TABLE tokenconfigs {
    //     name standard; //"marble"_n
    //     string version; //v0.5.0
    //     EOSLIB_SERIALIZE(tokenconfigs, (standard)(version))
    // };
    // typedef singleton<"tokenconfigs"_n, tokenconfigs> tokenconfigs_table;

    //config table
    //scope: self
    TABLE config {
        string contract_name;
        string contract_version;
        name admin;
        uint64_t last_serial;
        EOSLIB_SERIALIZE(config, (contract_name)(contract_version)(admin)(last_serial))
    };
    typedef singleton<name("config"), config> config_table;

    //groups table
    //scope: self
    TABLE group {
        string title;
        string description;
        name group_name;
        name manager;
        uint64_t supply;
        uint64_t issued_supply;
        uint64_t supply_cap;

        uint64_t primary_key() const { return group_name.value; }
        EOSLIB_SERIALIZE(group, (title)(description)(group_name)(manager)
            (supply)(issued_supply)(supply_cap))
    };
    typedef multi_index<name("groups"), group> groups_table;

    //behaviors table
    //scope: group
    TABLE behavior {
        name behavior_name;
        bool state;
        uint64_t primary_key() const { return behavior_name.value; }
        EOSLIB_SERIALIZE(behavior, (behavior_name)(state))
    };
    typedef multi_index<name("behaviors"), behavior> behaviors_table;

    //items table
    //scope: self
    TABLE item {
        uint64_t serial;
        name group;
        name owner;
        uint64_t primary_key() const { return serial; }
        uint64_t by_group() const { return group.value; }
        uint64_t by_owner() const { return owner.value; }
        EOSLIB_SERIALIZE(item, (serial)(group)(owner))
    };
    typedef multi_index<name("items"), item,
        indexed_by<"bygroup"_n, const_mem_fun<item, uint64_t, &item::by_group>>,
        indexed_by<"byowner"_n, const_mem_fun<item, uint64_t, &item::by_owner>>
    > items_table;

    //tags table
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

    //attributes table
    //scope: serial
    TABLE attribute {
        name attribute_name;
        int64_t points;

        uint64_t primary_key() const { return attribute_name.value; }
        EOSLIB_SERIALIZE(attribute, (attribute_name)(points))
    };
    typedef multi_index<name("attributes"), attribute> attributes_table;

    //events table
    //scope: serial
    TABLE event {
        name event_name;
        time_point_sec event_time;

        uint64_t primary_key() const { return event_name.value; }
        EOSLIB_SERIALIZE(event, (event_name)(event_time))
    };
    typedef multi_index<name("events"), event> events_table;

    //subgroups table
    //scope: serial
    // TABLE subgroup {
    //     name subgroup_name;
    //     uint64_t primary_key() const { return subgroup_name.value; }
    //     EOSLIB_SERIALIZE(subgroup, (subgroup_name))
    // };
    // typedef multi_index<name("subgroups"), subgroup> subgroups_table;

    //frames table
    //scope: self
    TABLE frame {
        name frame_name;
        name group;
        map<name, string> default_tags; //tag_name => default content
        map<name, int64_t> default_attributes; //attribute_name => default value
        // map<name, time_point_sec> default_events; //event_name => default value

        uint64_t primary_key() const { return frame_name.value; }
        uint64_t by_group() const { return group.value; }
        EOSLIB_SERIALIZE(frame, (frame_name)(group)(default_tags)(default_attributes))
    };
    typedef multi_index<"frames"_n, frame,
        indexed_by<"bygroup"_n, const_mem_fun<frame, uint64_t, &frame::by_group>>
    > frames_table;

};