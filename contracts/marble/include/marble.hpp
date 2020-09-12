// Marble is a Digital Item format for EOSIO software.
//
// author: Craig Branscom
// company: Dappetizer, LLC
// contract: marble
// version: v0.3.0

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

    //======================== config actions ========================

    //initialize the contract
    //auth: self
    ACTION init(string contract_name, string contract_version, name initial_admin);

    //set new contract version
    //auth: admin
    ACTION setversion(string new_version);

    //set new admin
    //auth: admin
    ACTION setadmin(name new_admin);

    //assume RAM costs for a table row
    //auth: payer
    //ACTION payram(name payer, name table_name, name key);

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
    ACTION togglebhvr(name group_name, name behavior_name);

    //lock a behavior to prevent mutations
    //auth: manager
    ACTION lockbhvr(name group_name, name behavior_name);

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

    //reclaim an item from the owner
    //auth: manager
    ACTION reclaimitem(uint64_t serial);

    //consume an item
    //post: inline releaseall() if bond(s) exist
    //auth: owner
    ACTION consumeitem(uint64_t serial);

    //destroy an item
    //post: inline releaseall() if bond(s) exist
    //auth: manager
    ACTION destroyitem(uint64_t serial, string memo);

    //freeze an item to prevent transfer, activate, consume, reclaim, or destroy
    //auth: manager
    // ACTION freezeitem(uint64_t serial);

    //unfreeze an item
    //auth: manager
    // ACTION unfreezeitem(uint64_t serial);

    //======================== tag actions ========================

    //assign a new tag to an item
    //auth: manager
    ACTION newtag(uint64_t serial, name tag_name, string content, optional<string> checksum, optional<string> algorithm, bool shared);

    //update tag content, checksum, and/or algorithm
    //auth: manager
    ACTION updatetag(uint64_t serial, name tag_name, string new_content, optional<string> new_checksum, optional<string> new_algorithm, bool shared);

    //lock a tag to prevent mutations
    //auth: manager
    ACTION locktag(uint64_t serial, name tag_name, bool shared);

    //remove tag from item
    //auth: manager
    ACTION rmvtag(uint64_t serial, name group_name, name tag_name, string memo, bool shared);

    //======================== attribute actions ========================

    //assign a new attribute to an item
    //auth: manager
    ACTION newattribute(uint64_t serial, name attribute_name, int64_t initial_points, bool shared);

    //sets an attributes points
    //auth: manager
    ACTION setpoints(uint64_t serial, name attribute_name, int64_t new_points, bool shared);

    //increases attribute points by amount
    //auth: manager
    ACTION increasepts(uint64_t serial, name attribute_name, uint64_t points_to_add, bool shared);

    //decreases attribute points by amount
    //auth: manager
    ACTION decreasepts(uint64_t serial, name attribute_name, uint64_t points_to_subtract, bool shared);

    //locks an attribute to prevent mutations
    //auth: manager
    ACTION lockattr(uint64_t serial, name attribute_name, bool shared);

    //removes an attribute from an item
    //auth: manager
    ACTION rmvattribute(uint64_t serial, name group_name, name attribute_name, bool shared);

    //======================== event actions ========================

    //create a new event (if no custom event time given use current time point)
    //auth: manager
    ACTION newevent(uint64_t serial, name event_name, optional<time_point_sec> custom_event_time, bool shared);

    //set a custom time on an event
    //auth: manager
    ACTION seteventtime(uint64_t serial, name event_name, time_point_sec new_event_time, bool shared);

    //lock an event time to prevent mutations
    //auth: manager
    ACTION lockevent(uint64_t serial, name event_name, bool shared);

    //remove an event
    //auth: manager
    ACTION rmvevent(uint64_t serial, name group_name, name event_name, bool shared);

    //log an event (will not save to events table)
    //auth: self
    ACTION logevent(name event_name, int64_t event_value, time_point_sec event_time, string memo, bool shared);

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

    //cleans a frame from an item
    //auth: manager
    ACTION cleanframe(name frame_name, uint64_t serial);

    //remove a frame
    //auth: manager
    ACTION rmvframe(name frame_name, string memo);

    //======================== bond actions ========================

    //back an item with a fungible token (draws from manager wallet balance)
    //auth: manager
    ACTION newbond(uint64_t serial, asset amount, optional<name> release_event);

    //add more tokens to an existing bond
    //pre: bond exists, bond not locked
    //auth: manager
    ACTION addtobond(uint64_t serial, asset amount);

    //release preconfigured amount from item bond
    //pre: item exists, release conditions met
    //auth: item owner
    ACTION release(uint64_t serial);

    //release all bond amounts from an item
    //pre: item consumed or destroyed, release_to == item.owner
    //auth: contract (inline)
    ACTION releaseall(uint64_t serial, name release_to);

    //locks a bond to prevent settings changes
    //pre: bond not locked
    //auth: manager
    ACTION lockbond(uint64_t serial);

    //======================== wallet actions ========================

    //withdraw tokens from a wallet
    //auth: wallet owner
    ACTION withdraw(name wallet_owner, asset amount);

    //======================== notification handlers ========================

    //catch a transfer from eosio.token
    [[eosio::on_notify("eosio.token::transfer")]]
    void catch_transfer(name from, name to, asset quantity, string memo);

    //======================== contract tables ========================

    //config table
    //scope: self
    //ram payer: contract
    TABLE config {
        string contract_name;
        string contract_version;
        name admin;
        uint64_t last_serial;
        //symbol core_sym;
        //vector<name> installed;

        EOSLIB_SERIALIZE(config, (contract_name)(contract_version)(admin)(last_serial))
    };
    typedef singleton<name("config"), config> config_table;

    //groups table
    //scope: self
    //ram payer: admin
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
    //ram payer: manager
    TABLE behavior {
        name behavior_name;
        bool state;
        bool locked;

        uint64_t primary_key() const { return behavior_name.value; }

        EOSLIB_SERIALIZE(behavior, (behavior_name)(state)(locked))
    };
    typedef multi_index<name("behaviors"), behavior> behaviors_table;

    //items table
    //scope: self
    //ram payer: manager
    TABLE item {
        uint64_t serial;
        name group;
        name owner;
        //uint64_t edition;

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
    //ram payer: manager
    TABLE tag {
        name tag_name;
        string content;
        string checksum;
        string algorithm;
        bool locked;

        uint64_t primary_key() const { return tag_name.value; }

        EOSLIB_SERIALIZE(tag, (tag_name)(content)(checksum)(algorithm)(locked))
    };
    typedef multi_index<"tags"_n, tag> tags_table;

    //shared tags table
    //scope: group
    //ram payer: manager
    TABLE shared_tag {
        name tag_name;
        string content;
        string checksum;
        string algorithm;
        bool locked;

        uint64_t primary_key() const { return tag_name.value; }

        EOSLIB_SERIALIZE(shared_tag, (tag_name)(content)(checksum)(algorithm)(locked))
    };
    typedef multi_index<name("sharedtags"), shared_tag> shared_tags_table;

    //attributes table
    //scope: serial
    //ram payer: manager
    TABLE attribute {
        name attribute_name;
        int64_t points;
        bool locked;

        uint64_t primary_key() const { return attribute_name.value; }

        EOSLIB_SERIALIZE(attribute, (attribute_name)(points)(locked))
    };
    typedef multi_index<name("attributes"), attribute> attributes_table;

    //shared attributes table
    //scope: group
    //ram payer: manager
    TABLE shared_attribute {
        name attribute_name;
        int64_t points;
        bool locked;

        uint64_t primary_key() const { return attribute_name.value; }

        EOSLIB_SERIALIZE(shared_attribute, (attribute_name)(points)(locked))
    };
    typedef multi_index<name("sharedattrs"), shared_attribute> shared_attributes_table;

    //events table
    //scope: serial
    //ram payer: manager
    TABLE event {
        name event_name;
        time_point_sec event_time;
        bool locked;

        uint64_t primary_key() const { return event_name.value; }

        EOSLIB_SERIALIZE(event, (event_name)(event_time)(locked))
    };
    typedef multi_index<name("events"), event> events_table;

    //shared events table
    //scope: group
    //ram payer: manager
    TABLE shared_event {
        name event_name;
        time_point_sec event_time;
        bool locked;

        uint64_t primary_key() const { return event_name.value; }

        EOSLIB_SERIALIZE(shared_event, (event_name)(event_time)(locked))
    };
    typedef multi_index<name("sharedevents"), shared_event> shared_events_table;

    //frames table
    //scope: self
    //ram payer: manager
    TABLE frame {
        name frame_name;
        name group;
        map<name, string> default_tags; //tag_name => default content
        map<name, int64_t> default_attributes; //attribute_name => default value
        // map<name, time_point_sec> default_events; //event_name => default value
        // bool shared;

        uint64_t primary_key() const { return frame_name.value; }
        uint64_t by_group() const { return group.value; }
        EOSLIB_SERIALIZE(frame, (frame_name)(group)(default_tags)(default_attributes))
    };
    typedef multi_index<"frames"_n, frame,
        indexed_by<"bygroup"_n, const_mem_fun<frame, uint64_t, &frame::by_group>>
    > frames_table;

    //bonds table
    //scope: serial
    //ram payer: manager
    TABLE bond {
        asset backed_amount; //token amount stored by bond
        name release_event; //event name storing release time (blank for no release time)
        bool locked; //if true bond settings cannot be changed

        // asset per_release; //amount released from bond per release event
        // uint16_t steps; //number of release steps before maturity
        // asset per_step; //amount released from bond per step

        uint64_t primary_key() const { return backed_amount.symbol.code().raw(); }

        EOSLIB_SERIALIZE(bond, (backed_amount)(release_event)(locked))
    };
    typedef multi_index<name("bonds"), bond> bonds_table;

    //wallets table
    //scope: owner
    //ram payer: owner
    TABLE wallet {
        asset balance; //wallet balance

        uint64_t primary_key() const { return balance.symbol.code().raw(); }

        EOSLIB_SERIALIZE(wallet, (balance))
    };
    typedef multi_index<name("wallets"), wallet> wallets_table;

    //currencies table
    //scope: self
    //ram payer: contract
    TABLE currency {
        asset deposits; //total deposited assets across wallets
        uint32_t wallets; //total unique wallets with balance
        name account; //account where currency contract is deployed
        //asset bonded; //total assets tied up in bonds

        uint64_t primary_key() const { return deposits.symbol.code().raw(); }

        EOSLIB_SERIALIZE(currency, (deposits)(wallets)(account))
    };
    typedef multi_index<name("currencies"), currency> currencies_table;

};