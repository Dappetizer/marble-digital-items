#include "../include/marble.hpp"

// marble::marble(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {}

// marble::~marble() {}

//======================== admin actions ========================

ACTION marble::init(string initial_version, name initial_admin) {
    
    //authenticate
    require_auth(get_self());

    //open configs singleton
    configs_singleton configs(get_self(), get_self().value);

    //validate
    check(!configs.exists(), "config already initialized");
    check(is_account(initial_admin), "initial admin account doesn't exist");

    //initialize
    tokenconfigs new_conf = {
        "marble"_n, //standard
        initial_version, //version
        initial_admin, //admin
        0 //last_serial
    };

    //set new configs
    configs.set(new_conf, get_self());

    //TODO?: inline to newgroup()

}

ACTION marble::setversion(string new_version) {
    
    //open configs singleton, get config
    configs_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //set new version
    conf.version = new_version;

    //update configs singleton
    configs.set(conf, get_self());

}

ACTION marble::setadmin(name new_admin, string memo) {
    
    //open configs singleton, get config
    configs_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //set new admin
    conf.admin = new_admin;

    //update configs singleton
    configs.set(conf, get_self());

}

//======================== utility actions ========================

ACTION marble::logevent(name event_name, int64_t event_value, time_point_sec event_time, string memo) {

    //authenticate
    require_auth(get_self());

}

ACTION marble::paybwbill() {

    //authenticate
    require_auth(get_self());

}

//======================== group actions ========================

ACTION marble::newgroup(string title, string description, name group_name, name manager, uint64_t supply_cap) {
    
    //open configs singleton, get config
    configs_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //open groups table, search for group
    groups_table groups(get_self(), get_self().value);
    auto g_itr = groups.find(group_name.value);

    //validate
    check(g_itr == groups.end(), "group name already taken");
    check(supply_cap > 0, "supply cap must be greater than zero");
    check(is_account(manager), "manager account doesn't exist");

    //initialize
    map<name, bool> initial_behaviors;

    //group behaviors
    initial_behaviors["mintable"_n] = true;
    initial_behaviors["transferable"_n] = true;
    initial_behaviors["destructible"_n] = true;

    //tag behaviors
    initial_behaviors["taggable"_n] = true;
    initial_behaviors["updateable"_n] = true;

    //attribute behaviors
    initial_behaviors["attributable"_n] = true;
    initial_behaviors["increasable"_n] = true;
    initial_behaviors["decreasable"_n] = true;

    //emplace new group
    groups.emplace(get_self(), [&](auto& col) {
        col.title = title;
        col.description = description;
        col.group_name = group_name;
        col.manager = manager;
        col.supply = 0;
        col.issued_supply = 0;
        col.supply_cap = supply_cap;
        col.behaviors = initial_behaviors;
    });

}

ACTION marble::addbehavior(name group_name, name behavior_name, bool initial_value) {
    
    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& g = groups.get(group_name.value, "group not found");

    //authenticate
    require_auth(g.manager);

    //validate
    check(g.behaviors.find(behavior_name) == g.behaviors.end(), "behavior already exists");

    //initialize
    auto new_behaviors = g.behaviors;
    new_behaviors[behavior_name] = initial_value;

    groups.modify(g, same_payer, [&](auto& col) {
        col.behaviors = new_behaviors;
    });

}

ACTION marble::toggle(name group_name, name behavior_name, string memo) {
    
    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& g = groups.get(group_name.value, "group not found");

    //authenticate
    require_auth(g.manager);

    //validate
    check(g.behaviors.find(behavior_name) != g.behaviors.end(), "behavior not found");

    //initialize
    auto new_behaviors = g.behaviors;
    new_behaviors[behavior_name] = !new_behaviors.at(behavior_name);

    //update group behaviors
    groups.modify(g, same_payer, [&](auto& col) {
        col.behaviors = new_behaviors;
    });

}

ACTION marble::rmvbehavior(name group_name, name behavior_name) {
    
    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& g = groups.get(group_name.value, "group not found");

    //authenticate
    require_auth(g.manager);

    //initialize
    auto b_itr = g.behaviors.find(behavior_name);

    //validate
    check(b_itr != g.behaviors.end(), "behavior not found");

    //update group behaviors
    groups.modify(g, same_payer, [&](auto& col) {
        col.behaviors.erase(b_itr);
    });

}

ACTION marble::setmanager(name group_name, name new_manager, string memo) {
    
    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& g = groups.get(group_name.value, "group not found");

    //authenticate
    require_auth(g.manager);

    //validate
    check(is_account(new_manager), "new manager account doesn't exist");

    //update group
    groups.modify(g, same_payer, [&](auto& col) {
        col.manager = new_manager;
    });

}

//======================== nft actions ========================

ACTION marble::newnft(name owner, name group_name, bool log) {
    
    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& g = groups.get(group_name.value, "group name not found");

    //authenticate
    require_auth(g.manager);

    //validate
    check(g.supply + 1 <= g.supply_cap, "supply cap reached");

    //open configs singleton, get configs
    configs_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //initialize
    auto now = time_point_sec(current_time_point());
    uint64_t new_serial = conf.last_serial + 1;

    //increment last_serial, set new config
    conf.last_serial += 1;
    configs.set(conf, get_self());

    //open nfts table, find nft
    nfts_table nfts(get_self(), get_self().value);
    auto n = nfts.find(new_serial);

    //validate
    check(n == nfts.end(), "serial already exists. contact admin.");

    //emplace new nft
    nfts.emplace(get_self(), [&](auto& col) {
        col.serial = new_serial;
        col.group = group_name;
        col.owner = owner;
    });

    //update group
    groups.modify(g, same_payer, [&](auto& col) {
        col.supply += 1;
        col.issued_supply += 1;
    });

    //TODO: construct frame

    if (log) {

        //inline logevent
        action(permission_level{get_self(), name("active")}, get_self(), name("logevent"), make_tuple(
            "newserial"_n, //event_name
            new_serial, //event_value
            now, //event_time
            std::string("log new nft serial") //memo
        )).send();

    }

}

ACTION marble::transfernft(uint64_t serial, name new_owner, string memo) {

    //open nfts table, get nft
    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    //authenticate
    require_auth(n.owner);

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& g = groups.get(n.group.value, "group not found");

    //validate
    check(is_account(new_owner), "new owner account doesn't exist");
    check(g.behaviors.at("transferable"_n), "nft is not transferable");

    //initialize
    name sender = n.owner;

    //update nft
    nfts.modify(n, same_payer, [&](auto& col) {
        col.owner = new_owner;
    });

    //notify sender and new owner
    require_recipient(sender);
    require_recipient(new_owner);

}

ACTION marble::transfernfts(vector<uint64_t> serials, name new_owner, string memo) {

    //validate
    check(is_account(new_owner), "new owner account doesn't exist");

    //loop over serials
    for (uint64_t s : serials) {

        //open nfts table, get nft
        nfts_table nfts(get_self(), get_self().value);
        auto& n = nfts.get(s, "nft not found");

        //authenticate
        require_auth(n.owner);

        //open groups table, get group
        groups_table groups(get_self(), get_self().value);
        auto& g = groups.get(n.group.value, "group not found");

        //validate
        check(g.behaviors.at("transferable"_n), "nft is not transferable");

        //update nft
        nfts.modify(n, same_payer, [&](auto& col) {
            col.owner = new_owner;
        });

    }

    //notify new owner
    // require_recipient(sender);
    require_recipient(new_owner);

}

ACTION marble::destroynft(uint64_t serial, string memo) {

    //open nfts table, get nft
    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& g = groups.get(n.group.value, "group not found");

    //authenticate
    require_auth(g.manager);

    //validate
    check(g.behaviors.at("destructible"_n), "nft is not destructible");
    check(g.supply > 0, "cannot reduce supply below zero");

    //update group
    groups.modify(g, same_payer, [&](auto& col) {
        col.supply -= 1;
    });

    //erase nft
    nfts.erase(n);

}

//======================== tag actions ========================

ACTION marble::newtag(uint64_t serial, name tag_name, string content,
    optional<string> checksum, optional<string> algorithm) {

    //open tags table, search for tag
    tags_table tags(get_self(), serial);
    auto t_itr = tags.find(tag_name.value);

    //open nfts table, get nft
    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& g = groups.get(n.group.value, "group not found");

    //authenticate
    require_auth(g.manager);

    //validate
    check(tag_name != name(0), "tag name cannot be empty");
    check(t_itr == tags.end(), "tag name already exists on nft");
    check(g.behaviors.at("taggable"_n), "nft is not taggable");

    //initialize
    string chsum = "";
    string algo = "";

    if (checksum) {
        chsum = *checksum;
    }

    if (algorithm) {
        algo = *algorithm;
    }

    //emplace tag
    tags.emplace(get_self(), [&](auto& col) {
        col.tag_name = tag_name;
        col.content = content;
        col.checksum = chsum;
        col.algorithm = algo;
    });

}

ACTION marble::updatetag(uint64_t serial, name tag_name, string new_content,
    optional<string> new_checksum, optional<string> new_algorithm) {

    //open nfts table, get nft
    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& g = groups.get(n.group.value, "group not found");

    //open tags table, get tag
    tags_table tags(get_self(), serial);
    auto& t = tags.get(tag_name.value, "tag not found on nft");

    //authenticate
    require_auth(g.manager);

    //validate
    check(g.behaviors.at("updateable"_n), "tag not updateable");

    string new_chsum = "";
    string new_algo = t.algorithm;

    if (new_checksum) {
        new_chsum = *new_checksum;
    }

    if (new_algorithm) {
        new_algo = *new_algorithm;
    }

    //update tag
    tags.modify(t, same_payer, [&](auto& col) {
        col.content = new_content;
        col.checksum = new_chsum;
        col.algorithm = new_algo;
    });

}

ACTION marble::rmvtag(uint64_t serial, name tag_name, string memo) {

    //open nfts table, get nft
    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& g = groups.get(n.group.value, "group not found");

    //open tags table, get tag
    tags_table tags(get_self(), serial);
    auto& t = tags.get(tag_name.value, "tag not found on nft");

    //authenticate
    require_auth(g.manager);

    //erase nft
    tags.erase(t);

}

//======================== attribute actions ========================

ACTION marble::newattribute(uint64_t serial, name attribute_name, int64_t initial_points) {

    //open nfts table, get nft
    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& g = groups.get(n.group.value, "group not found");

    //open attributes table, get attribute
    attributes_table attributes(get_self(), serial);
    auto a = attributes.find(attribute_name.value);

    //authenticate
    require_auth(g.manager);

    //validate
    check(a == attributes.end(), "attribute name already exists for nft");

    //emplace new tattribute
    attributes.emplace(g.manager, [&](auto& col) {
        col.attribute_name = attribute_name;
        col.points = initial_points;
    });

}

ACTION marble::setpoints(uint64_t serial, name attribute_name, int64_t new_points) {
    
    //open nfts table, get nft
    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& g = groups.get(n.group.value, "group not found");

    //open attributes table, get attribute
    attributes_table attributes(get_self(), serial);
    auto& a = attributes.get(attribute_name.value, "attribute not found");

    //authenticate
    require_auth(g.manager);

    //set new attribute points
    attributes.modify(a, same_payer, [&](auto& col) {
        col.points = new_points;
    });

}

ACTION marble::increasepts(uint64_t serial, name attribute_name, uint64_t points_to_add) {
    
    //open nfts table, get nft
    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& g = groups.get(n.group.value, "group not found");

    //open attributes table, get attribute
    attributes_table attributes(get_self(), serial);
    auto& a = attributes.get(attribute_name.value, "attribute not found");

    //authenticate
    require_auth(g.manager);

    //validate
    check(g.behaviors.at("increasable"_n), "attribute not increasable");
    check(points_to_add > 0, "must add greater than zero points");

    //modify attribute points
    attributes.modify(a, same_payer, [&](auto& col) {
        col.points += points_to_add;
    });

}

ACTION marble::decreasepts(uint64_t serial, name attribute_name, uint64_t points_to_subtract) {

    //open nfts table, get nft
    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& g = groups.get(n.group.value, "group not found");

    //open attributes table, get attribute
    attributes_table attributes(get_self(), serial);
    auto& a = attributes.get(attribute_name.value, "attribute not found");

    //authenticate
    require_auth(g.manager);

    //validate
    check(g.behaviors.at("decreasable"_n), "nft not decreasable");
    check(points_to_subtract > 0, "must subtract greater than zero points");

    //modify attribute points
    attributes.modify(a, same_payer, [&](auto& col) {
        col.points -= points_to_subtract;
    });

}

ACTION marble::rmvattribute(uint64_t serial, name attribute_name) {

    //open nfts table, get nft
    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& g = groups.get(n.group.value, "group not found");

    //open attributes table, get attribute
    attributes_table attributes(get_self(), serial);
    auto a = attributes.find(attribute_name.value);

    //authenticate
    require_auth(g.manager);

    //validate
    check(a != attributes.end(), "attribute not found");

    //erase attribute
    attributes.erase(a);

}

//======================== frame actions ========================

ACTION marble::newframe(name frame_name, name group, map<name, string> default_tags, map<name, int64_t> default_attributes) {

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& g = groups.get(group.value, "group not found");

    //authenticate
    require_auth(g.manager);

    //open frames table, find frame
    frames_table frames(get_self(), get_self().value);
    auto f = frames.find(frame_name.value);

    //validate
    check(f == frames.end(), "frame already exists");

    //emplace new frame
    frames.emplace(g.manager, [&](auto& col) {
        col.frame_name = frame_name;
        col.group = group;
        col.default_tags = default_tags;
        col.default_attributes = default_attributes;
    });

}

ACTION marble::applyframe(name frame_name, uint64_t serial, bool overwrite) {

    //open frames table, get frame
    frames_table frames(get_self(), get_self().value);
    auto& f = frames.get(frame_name.value, "frame not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& g = groups.get(f.group.value, "group not found");

    //authenticate
    require_auth(g.manager);

    //apply tags
    for (auto itr = f.default_tags.begin(); itr != f.default_tags.end(); itr++) {

        //open tags table, find tag
        tags_table tags(get_self(), serial);
        auto t_itr = tags.find(itr->first.value);

        //NOTE: will skip existing tag with same tag name if overwrite is false

        if (t_itr == tags.end()) {
            
            //emplace new tag
            tags.emplace(g.manager, [&](auto& col) {
                col.tag_name = itr->first;
                col.content = itr->second;
                col.checksum = "";
                col.algorithm = "";
            });

        } else if (overwrite) {

            //overwrite existing tag
            tags.modify(t_itr, same_payer, [&](auto& col) {
                col.content = itr->second;
                col.checksum = "";
                col.algorithm = "";
            });

        }

    }

    //apply attributes
    for (auto itr = f.default_attributes.begin(); itr != f.default_attributes.end(); itr++) {

        //open attributes table, find attribute
        attributes_table attributes(get_self(), serial);
        auto a_itr = attributes.find(itr->first.value);

        //NOTE: will skip existing attribute with same attribute name if overwrite is false

        if (a_itr == attributes.end()) {
            
            //emplace new attribute
            attributes.emplace(g.manager, [&](auto& col) {
                col.attribute_name = itr->first;
                col.points = itr->second;
            });

        } else if (overwrite) {

            //overwrite existing attribute
            attributes.modify(a_itr, same_payer, [&](auto& col) {
                col.points = itr->second;
            });

        }

    }

}

ACTION marble::rmvframe(name frame_name, string memo) {

    //open frames table, get frame
    frames_table frames(get_self(), get_self().value);
    auto& f = frames.get(frame_name.value, "frame not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& g = groups.get(f.group.value, "group not found");

    //authenticate
    require_auth(g.manager);

    //erase frame
    frames.erase(f);

}