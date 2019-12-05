// Marble is a lightweight NFT format for EOSIO software.
//
// author: Craig Branscom
// company: Dappetizer, LLC
// contract: marble
// version: v0.4.0

#include <eosio/eosio.hpp>
#include <eosio/action.hpp>
#include <eosio/singleton.hpp>

using namespace std;
using namespace eosio;

//TODO?: make checksum and algorithm fields optional
//TODO?: make contract ram payer for everything

//TODO: implement transfernfts

CONTRACT marble : public contract {

    public:

    marble(name self, name code, datastream<const char*> ds);

    ~marble();

    //group behaviors: mintable, transferable, destructible

    //tag behaviors: taggable, updateable

    //attribute behaviors: attributable, increasable, decreasable

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
    using logevent_action = action_wrapper<"logevent"_n, &marble::logevent>;

    //pay bandwidth bill
    ACTION paybwbill();
    using paybwbill_action = action_wrapper<"paybwbill"_n, &marble::paybwbill>;

    //======================== group actions ========================

    //creates a new nft group
    ACTION newgroup(string title, string description, name group_name, 
        name manager, uint64_t supply_cap);
    using newgroup_action = action_wrapper<"newgroup"_n, &marble::newgroup>;

    //adds a behavior to a group
    ACTION addbehavior(name group_name, name behavior_name, bool initial_value);
    using addbehavior_action = action_wrapper<"addbehavior"_n, &marble::addbehavior>;

    //toggles a group behavior on/off
    ACTION toggle(name group_name, name behavior_name, string memo);
    using toggle_action = action_wrapper<"toggle"_n, &marble::toggle>;

    //removes a behavior from a group
    ACTION rmvbehavior(name group_name, name behavior_name);
    using rmvbehavior_action = action_wrapper<"rmvbehavior"_n, &marble::rmvbehavior>;

    //sets a new group manager
    ACTION setmanager(name group_name, name new_manager, string memo);
    using setmanager_action = action_wrapper<"setmanager"_n, &marble::setmanager>;

    //======================== nft actions ========================

    //creates a new nft
    ACTION newnft(name owner, name group_name, bool log);
    using newnft_action = action_wrapper<"newnft"_n, &marble::newnft>;

    //transfers ownership of a single nft
    ACTION transfernft(uint64_t serial, name new_owner, string memo);
    using transfernft_action = action_wrapper<"transfernft"_n, &marble::transfernft>;

    //transfers ownership of one or more nfts
    ACTION transfernfts(vector<uint64_t> serials, name new_owner, string memo);
    using transfernfts_action = action_wrapper<"transfernfts"_n, &marble::transfernfts>;

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
    ACTION rmvtag(uint64_t serial, name tag_name, string memo);
    using removetag_action = action_wrapper<"rmvtag"_n, &marble::rmvtag>;

    //======================== attribute actions ========================

    //assign a new attribute to an nft
    ACTION newattribute(uint64_t serial, name attribute_name, int64_t initial_points);
    using newattribute_action = action_wrapper<"newattribute"_n, &marble::newattribute>;

    //sets an attributes points
    ACTION setpoints(uint64_t serial, name attribute_name, int64_t new_points);
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

    //======================== frame actions ========================

    //set up a new frame
    ACTION newframe(name frame_name, name group, map<name, string> default_tags, map<name, int64_t> default_attributes);
    using newframe_action = action_wrapper<"newframe"_n, &marble::newframe>;

    //applies a frame to an nft
    ACTION applyframe(name frame_name, uint64_t serial, bool overwrite);
    using applyframe_action = action_wrapper<"applyframe"_n, &marble::applyframe>;

    //remove a frame
    ACTION rmvframe(name frame_name, string memo);
    using rmvframe_action = action_wrapper<"rmvframe"_n, &marble::rmvframe>;

    //======================== contract tables ========================

    //raw contract ram: ~784,622 bytes

    //contract configs
    //scope: singleton
    //ram: ~255 bytes
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
    //ram: variable
    TABLE group {
        string title;
        string description;
        name group_name;
        name manager;
        uint64_t supply;
        uint64_t issued_supply;
        uint64_t supply_cap;
        map<name, bool> behaviors;

        uint64_t primary_key() const { return group_name.value; }
        EOSLIB_SERIALIZE(group, 
            (title)(description)(group_name)(manager)
            (supply)(issued_supply)(supply_cap)(behaviors))
    };
    typedef multi_index<"groups"_n, group> groups_table;

    //individual nft data
    //scope: self
    //ram: ~392 bytes
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
    //ram: variable
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
    //ram: ~128 bytes
    TABLE attribute {
        name attribute_name;
        int64_t points;

        uint64_t primary_key() const { return attribute_name.value; }
        EOSLIB_SERIALIZE(attribute, (attribute_name)(points))
    };
    typedef multi_index<name("attributes"), attribute> attributes_table;

    //frames
    //scope: self
    //ram: variable
    TABLE frame {
        name frame_name;
        name group;
        map<name, string> default_tags; //tag_name => default_content
        map<name, int64_t> default_attributes; //attribute_name => default_value

        uint64_t primary_key() const { return frame_name.value; }
        uint64_t by_group() const { return group.value; }
        EOSLIB_SERIALIZE(frame, (frame_name)(group)(default_tags)(default_attributes))
    };
    typedef multi_index<"frames"_n, frame,
        indexed_by<"bygroup"_n, const_mem_fun<frame, uint64_t, &frame::by_group>>
    > frames_table;

};