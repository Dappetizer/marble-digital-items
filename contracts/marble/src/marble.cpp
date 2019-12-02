#include "../include/marble.hpp"

marble::marble(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {}

marble::~marble() {}

//======================== admin actions ========================

ACTION marble::init(string initial_version, name initial_admin) {
    
    //authenticate
    require_auth(get_self());

    configs_singleton configs(get_self(), get_self().value);

    //validate
    check(!configs.exists(), "contract config already initialized");
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

    //TODO: inline to newgroup()

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

ACTION marble::logevent(name event_name, uint64_t event_value, time_point_sec event_time, string memo) {

    //authenticate
    require_auth(get_self());

}

//======================== group actions ========================

ACTION marble::newgroup(string title, string description, name group_name, name manager, uint64_t supply_cap) {
    
    //open groups table, search for group
    groups_table groups(get_self(), get_self().value);
    auto g_itr = groups.find(group_name.value);
    
    //authenticate
    require_auth(manager);

    //validate
    check(g_itr == groups.end(), "group name already taken");
    check(supply_cap > 0, "supply cap must be greater than zero");

    //initialize
    map<name, bool> initial_settings;

    //core settings
    initial_settings["mintable"_n] = true;
    initial_settings["transferable"_n] = false;
    initial_settings["destructible"_n] = false;

    //tag settings
    // initial_settings["taggable"_n] = false;
    initial_settings["updateable"_n] = false;

    //attribute settings
    // initial_settings["attributable"_n] = false;
    initial_settings["increasable"_n] = false;
    initial_settings["decreasable"_n] = false;

    //emplace new group
    groups.emplace(get_self(), [&](auto& col) {
        col.group_name = group_name;
        col.manager = manager;
        col.title = title;
        col.description = description;
        col.supply = 0;
        col.issued_supply = 0;
        col.supply_cap = supply_cap;
        col.settings = initial_settings;
    });

}

ACTION marble::addsetting(name group_name, name setting_name, bool initial_value) {
    
    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& g = groups.get(group_name.value, "group not found");

    //authenticate
    require_auth(g.manager);

    //validate
    check(g.settings.find(setting_name) == g.settings.end(), "setting already exists");

    //initialize
    auto new_settings = g.settings;
    new_settings[setting_name] = initial_value;

    groups.modify(g, same_payer, [&](auto& col) {
        col.settings = new_settings;
    });

}

ACTION marble::toggle(name group_name, name setting_name, string memo) {
    
    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& g = groups.get(group_name.value, "group not found");

    //authenticate
    require_auth(g.manager);

    //validate
    check(g.settings.find(setting_name) != g.settings.end(), "setting not found");

    //initialize
    auto new_settings = g.settings;
    new_settings[setting_name] = !new_settings.at(setting_name);

    //update group options
    groups.modify(g, same_payer, [&](auto& col) {
        col.settings = new_settings;
    });

}

ACTION marble::rmvsetting(name group_name, name setting_name) {
    
    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& g = groups.get(setting_name.value, "group not found");

    //authenticate
    require_auth(g.manager);

    auto s_itr = g.settings.find(setting_name);

    //validate
    check(s_itr != g.settings.end(), "setting not found");

    //update group options
    groups.modify(g, same_payer, [&](auto& col) {
        col.settings.erase(s_itr);
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

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& g = groups.get(n.group.value, "group not found");

    //authenticate
    require_auth(g.manager);

    //validate
    check(is_account(new_owner), "new owner account doesn't exist");
    check(g.settings.at("transferable"_n), "nft is not transferable");

    //update nft
    nfts.modify(n, same_payer, [&](auto& col) {
        col.owner = new_owner;
    });

    //notify new owner
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
    check(g.settings.at("destructible"_n), "nft is not destructible");
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

    //validate
    check(tag_name != name(0), "tag name cannot be empty");
    check(t_itr == tags.end(), "tag name already exists on nft");

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
    check(g.settings.at("updateable"_n), "tag not updateable");

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

ACTION marble::newattribute(uint64_t serial, name attribute_name, uint64_t initial_points) {

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
    check(initial_points >= 0, "initial points cannot be negative");

    //emplace new tattribute
    attributes.emplace(g.manager, [&](auto& col) {
        col.attribute_name = attribute_name;
        col.points = initial_points;
    });

}

ACTION marble::setpoints(uint64_t serial, name attribute_name, uint64_t new_points) {
    
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
    check(new_points > 0, "new points greater than zero");

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
    check(g.settings.at("increasable"_n), "attribute not increasable");
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
    check(g.settings.at("decreasable"_n), "nft not decreasable");
    check(points_to_subtract > 0, "must subtract greater than zero points");
    check(points_to_subtract <= a.points, "cannot subtract points below zero");

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