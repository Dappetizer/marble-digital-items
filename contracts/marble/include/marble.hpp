// Marble is a modular digital item format for EOSIO software.
//
// author: Craig Branscom
// company: Dappetizer, LLC
// contract: marble
// version: v1.2.0

#include <eosio/eosio.hpp>
#include <eosio/action.hpp>
#include <eosio/singleton.hpp>
#include <eosio/asset.hpp>

using namespace std;
using namespace eosio;

//TODO: reclaim, freeze, update, release
//TODO: create release perm, linkauth to releaseall() action
//TODO: add core_symbol to config table
//TODO: create_or_update_account() function
//TODO?: add string payload_json to trigger

CONTRACT marble : public contract {

    public:

    marble(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {};
    ~marble() {};

    //constants
    const symbol CORE_SYM = symbol("TLOS", 4);
    const name MINT = name("mint");
    const name TRANSFER = name("transfer");
    const name ACTIVATE = name("activate");
    const name CONSUME = name("consume");
    const name DESTROY = name("destroy");

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

    //consume an item
    //auth: owner
    ACTION consumeitem(uint64_t serial);

    //reclaim an item from the owner
    //auth: manager
    // ACTION reclaimitem(uint64_t serial);

    //destroy an item
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
    ACTION newtag(uint64_t serial, name tag_name, string content, optional<string> checksum, optional<string> algorithm);

    //update tag content, checksum, and/or algorithm
    //auth: manager
    ACTION updatetag(uint64_t serial, name tag_name, string new_content, optional<string> new_checksum, optional<string> new_algorithm);

    //lock a tag to prevent mutations
    //auth: manager
    ACTION locktag(uint64_t serial, name tag_name);

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

    //locks an attribute to prevent mutations
    //auth: manager
    ACTION lockattr(uint64_t serial, name attribute_name);

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

    //lock an event time to prevent mutations
    //auth: manager
    ACTION lockevent(uint64_t serial, name event_name);

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

    //======================== backing actions ========================

    //back an item with a fungible token (draws from manager deposit balance)
    //auth: manager
    ACTION newbacking(uint64_t serial, asset amount, optional<name> release_auth, optional<asset> per_release);

    //release configured amount from item backing
    //auth: release_auth
    ACTION release(uint64_t serial, symbol token_symbol, name release_to);

    //release all backings from an item
    //auth: release_auth
    // ACTION releaseall(uint64_t serial, name release_to);

    //locks a backing to prevent release
    //auth: manager
    // ACTION lockbacking(uint64_t serial, symbol token_symbol);

    //======================== trigger actions ========================

    //create a new trigger for an item to be executed on behavior calls
    //auth: manager
    // ACTION newtrigger(uint64_t serial, name behavior_name, name action_name, vector<char> action_payload, optional<uint16_t> total_execs);

    //toggle primed state of trigger on/off
    //auth: manager
    // ACTION toggleprimed(uint64_t serial, name behavior_name);

    //execute a trigger
    //auth: contract
    // ACTION exectrigger(uint64_t serial, name behavior_name);

    //remove a trigger form an item
    //auth: manager
    // ACTION rmvtrigger(uint64_t serial, name behavior_name);

    //======================== account actions ========================

    //withdraw tokens from a deposit account
    //auth: account_owner
    ACTION withdraw(name account_owner, asset amount);

    //======================== notification handlers ========================

    //catch a transfer from eosio.token
    [[eosio::on_notify("eosio.token::transfer")]]
    void catch_transfer(name from, name to, asset quantity, string memo);

    //======================== contract tables ========================

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
        bool locked;
        uint64_t primary_key() const { return behavior_name.value; }
        EOSLIB_SERIALIZE(behavior, (behavior_name)(state)(locked))
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
        bool locked;
        uint64_t primary_key() const { return tag_name.value; }
        EOSLIB_SERIALIZE(tag, (tag_name)(content)(checksum)(algorithm)(locked))
    };
    typedef multi_index<"tags"_n, tag> tags_table;

    //attributes table
    //scope: serial
    TABLE attribute {
        name attribute_name;
        int64_t points;
        bool locked;
        uint64_t primary_key() const { return attribute_name.value; }
        EOSLIB_SERIALIZE(attribute, (attribute_name)(points)(locked))
    };
    typedef multi_index<name("attributes"), attribute> attributes_table;

    //events table
    //scope: serial
    TABLE event {
        name event_name;
        time_point_sec event_time;
        bool locked;
        uint64_t primary_key() const { return event_name.value; }
        EOSLIB_SERIALIZE(event, (event_name)(event_time)(locked))
    };
    typedef multi_index<name("events"), event> events_table;

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

    //backings table
    //scope: serial
    TABLE backing {
        asset backing_amount; //amount stored by backing
        // name release_acct;
        name release_auth; //account name that can authorize release (self for triggers)
        asset per_release; //amount released from backing per release call
        bool locked; //if true backing cannot be released

        uint64_t primary_key() const { return backing_amount.symbol.code().raw(); }
        EOSLIB_SERIALIZE(backing, (backing_amount)(release_auth)(per_release)(locked))
    };
    typedef multi_index<name("backings"), backing> backings_table;

    //triggers table
    //scope: serial
    // TABLE trigger {
    //     name behavior_name; //behavior that executes trigger

    //     // pair<char, char> comparator; //<, >, ==, !=, <=, >= (LESS_THAN, GREATER_THAN, EQUAL_TO, NOT_EUQAL_TO, LESS_THAN_EQUAL_TO, GREATER_THAN_EQUAL_TO)
    //     // variant<string, int64_t, time_point_sec, asset> conditions; //item values to compare before execution

    //     name action_name; //name of action to execute as inline
    //     vector<char> action_payload; //action payload as packed data

    //     bool primed; //if true will execute when triggered, if false will ignore
    //     uint16_t remaining_execs; //decrements upon trigger execution
    //     bool auto_prime; //automatically primes trigger again after execution
    //     bool auto_erase; //erases trigger after remaining_execs reach zero

    //     uint64_t primary_key() const { return behavior_name.value; }
    //     EOSLIB_SERIALIZE(trigger, (behavior_name))
    // };
    // typedef multi_index<name("triggers"), trigger> triggers_table;

    //accounts table
    //scope: account
    TABLE account {
        asset balance; //account balance amount

        uint64_t primary_key() const { return balance.symbol.code().raw(); }
        EOSLIB_SERIALIZE(account, (balance))
    };
    typedef multi_index<name("accounts"), account> accounts_table;

    //deposits table
    //scope: self //TODO?: contract_account
    // TABLE deposits {
    //     asset total_deposits; //total deposited assets across accounts
    //     uint32_t unique_accounts; //total unique accounts with balance
    //     uint64_t primary_key() const { return total_deposits.symbol.code().raw(); }
    //     EOSLIB_SERIALZIE(deposit, (total_deposits)(unique_accounts))
    // };
    // typedef multi_index<name("deposits"), deposit> deposits_table;

};