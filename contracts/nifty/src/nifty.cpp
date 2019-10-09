#include "../include/nifty.hpp"

nifty::nifty(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {}

nifty::~nifty() {}

//======================== admin actions ========================

ACTION nifty::init(string initial_version) {
    
    //authenticate
    require_auth(get_self());

    config_singleton configs(get_self(), get_self().value);

    //validate
    check(!configs.exists(), "contract config already initialized");

    config new_conf = {
        initial_version, //nifty_version
        get_self(), //admin_name
        0 //last_serial
    };

    configs.set(new_conf, get_self());

}

ACTION nifty::setversion(string new_version) {

    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin_name);

    conf.nifty_version = new_version;

    configs.set(conf, get_self());

}

ACTION nifty::setadmin(name new_admin, string memo) {

    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin_name);

    conf.admin_name = new_admin;

    configs.set(conf, get_self());

}

//======================== set actions ========================

ACTION nifty::newset(name set_name, name manager, string title, string description, uint64_t supply_cap) {

    //authenticate
    require_auth(manager);

    sets_table sets(get_self(), get_self().value);
    auto s_itr = sets.find(set_name.value);

    //validate
    check(s_itr == sets.end(), "set name already taken");
    check(supply_cap > 0, "supply cap must be greater than zero");

    map<name, bool> initial_options;
    initial_options["transferable"_n] = false;
    initial_options["destructible"_n] = false;
    initial_options["updateable"_n] = false;
    initial_options["upradeable"_n] = false;

    sets.emplace(get_self(), [&](auto& col) {
        col.set_name = set_name;
        col.manager = manager;
        col.title = title;
        col.description = description;
        col.supply = 0;
        col.issued_supply = 0;
        col.supply_cap = supply_cap;
        col.options = initial_options;
    });

}

ACTION nifty::addoption(name set_name, name option_name, bool initial_value) {

    sets_table sets(get_self(), get_self().value);
    auto& s = sets.get(set_name.value, "set not found");

    //authenticate
    require_auth(s.manager);

    auto opt_itr = s.options.find(option_name);

    //validate
    check(opt_itr == s.options.end(), "option already exists");

    auto new_options = s.options;
    new_options[option_name] = initial_value;

    sets.modify(s, same_payer, [&](auto& col) {
        col.options = new_options;
    });

}

ACTION nifty::toggle(name set_name, name option_name, string memo) {

    sets_table sets(get_self(), get_self().value);
    auto& s = sets.get(set_name.value, "set not found");

    //authenticate
    require_auth(s.manager);

    auto opt_itr = s.options.find(option_name);

    //validate
    check(opt_itr != s.options.end(), "option not found");

    auto new_options = s.options;
    new_options[option_name] = !new_options.at(option_name);

    sets.modify(s, same_payer, [&](auto& col) {
        col.options = new_options;
    });

}

ACTION nifty::rmvoption(name set_name, name option_name) {

    sets_table sets(get_self(), get_self().value);
    auto& s = sets.get(set_name.value, "set not found");

    //authenticate
    require_auth(s.manager);

    auto opt_itr = s.options.find(option_name);

    //validate
    check(opt_itr != s.options.end(), "option not found");

    sets.modify(s, same_payer, [&](auto& col) {
        col.options.erase(opt_itr);
    });

}

ACTION nifty::setmanager(name set_name, name new_manager, string memo) {

    sets_table sets(get_self(), get_self().value);
    auto& s = sets.get(set_name.value, "set not found");

    //authenticate
    require_auth(s.manager);

    //validate
    check(is_account(new_manager), "new manager account doesn't exist");

    sets.modify(s, same_payer, [&](auto& col) {
        col.manager = new_manager;
    });

}

//======================== nft actions ========================

ACTION nifty::newnft(name owner, name set_name, string content, 
    optional<string> checksum, optional<string> algorithm) {

    sets_table sets(get_self(), get_self().value);
    auto& s = sets.get(set_name.value, "set name not found");

    //authenticate
    require_auth(s.manager);

    //validate
    check(s.supply + 1 <= s.supply_cap, "supply cap reached");

    string initial_checksum = "";
    string initial_algo = "";

    if (checksum) {
        initial_checksum = *checksum;
    }

    if (algorithm) {
        initial_algo = *algorithm;
    }

    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    uint64_t new_serial = conf.last_serial + 1;
    conf.last_serial += 1;

    //set new config with updates last_serial
    configs.set(conf, get_self());

    nfts_table nfts(get_self(), get_self().value);
    auto n = nfts.find(new_serial);

    //validate
    check(n == nfts.end(), "serial already exists");

    //emplace new nft
    nfts.emplace(get_self(), [&](auto& col) {
        col.serial = new_serial;
        col.set_name = set_name;
        col.owner = owner;
        col.content = content;
        col.checksum = initial_checksum;
        col.algorithm = initial_algo;
    });

    //update set
    sets.modify(s, same_payer, [&](auto& col) {
        col.supply += 1;
        col.issued_supply += 1;
    });

}

ACTION nifty::updatenft(uint64_t serial, string content, 
    optional<string> checksum, optional<string> algorithm) {

    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    sets_table sets(get_self(), get_self().value);
    auto& s = sets.get(n.set_name.value, "set not found");

    //authenticate
    require_auth(s.manager);

    //validate
    check(s.options.at("updateable"_n) == true, "nft not updateable");

    string new_checksum = "";
    string new_algo = n.algorithm;

    if (checksum) {
        new_checksum = *checksum;
    }

    if (algorithm) {
        new_algo = *algorithm;
    }

    //update nft
    nfts.modify(n, same_payer, [&](auto& col) {
        col.content = content;
        col.checksum = new_checksum;
        col.algorithm = new_algo;
    });

}

ACTION nifty::transfernft(uint64_t serial, name new_owner, string memo) {

    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    sets_table sets(get_self(), get_self().value);
    auto& s = sets.get(n.set_name.value, "set not found");

    //authenticate
    require_auth(s.manager);

    //validate
    check(is_account(new_owner), "new owner account doesn't exist");
    check(s.options.at("transferable"_n) == true, "nft is not transferable");

    //update nft
    nfts.modify(n, same_payer, [&](auto& col) {
        col.owner = new_owner;
    });

    //notify new owner
    require_recipient(new_owner);

}

ACTION nifty::destroynft(uint64_t serial, string memo) {

    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    sets_table sets(get_self(), get_self().value);
    auto& s = sets.get(n.set_name.value, "set not found");

    //authenticate
    require_auth(s.manager);

    //validate
    check(s.options.at("destructible"_n) == true, "nft is not destructible");
    check(s.supply > 0, "cannot reduce below zero supply");

    //update set
    sets.modify(s, same_payer, [&](auto& col) {
        col.supply -= 1;
    });

    //erase nft
    nfts.erase(n);

}

//======================== attribute actions ========================

ACTION nifty::addattribute(uint64_t serial, name attribute_name, uint64_t initial_points) {

    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    sets_table sets(get_self(), get_self().value);
    auto& s = sets.get(n.set_name.value, "set not found");

    attributes_table attributes(get_self(), serial);
    auto a = attributes.find(attribute_name.value);

    //authenticate
    require_auth(s.manager);

    //validate
    check(initial_points >= 0, "initial points cannot be negative");
    check(a == attributes.end(), "attribute name already exists for nft");

    attributes.emplace(s.manager, [&](auto& col) {
        col.attribute_name = attribute_name;
        col.points = initial_points;
    });

}

ACTION nifty::increase(uint64_t serial, name attribute_name, uint64_t points_to_add) {

    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    sets_table sets(get_self(), get_self().value);
    auto& s = sets.get(n.set_name.value, "set not found");

    attributes_table attributes(get_self(), serial);
    auto& a = attributes.get(attribute_name.value, "attribute not found");

    //authenticate
    require_auth(s.manager);

    //validate
    check(s.options.at("upgradeable"_n), "nft not upgradeable");
    check(points_to_add > 0, "must add greater than zero points");

    attributes.modify(a, same_payer, [&](auto& col) {
        col.points += points_to_add;
    });

}

ACTION nifty::decrease(uint64_t serial, name attribute_name, uint64_t points_to_subtract) {

    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    sets_table sets(get_self(), get_self().value);
    auto& s = sets.get(n.set_name.value, "set not found");

    attributes_table attributes(get_self(), serial);
    auto& a = attributes.get(attribute_name.value, "attribute not found");

    //authenticate
    require_auth(s.manager);

    //validate
    check(s.options.at("upgradeable"_n), "nft not upgradeable");
    check(points_to_subtract > 0, "must remove greater than zero points");
    check(points_to_subtract <= a.points, "cannot subtract points below zero");

    attributes.modify(a, same_payer, [&](auto& col) {
        col.points -= points_to_subtract;
    });

}

ACTION nifty::rmvattribute(uint64_t serial, name attribute_name) {

    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    sets_table sets(get_self(), get_self().value);
    auto& s = sets.get(n.set_name.value, "set not found");

    attributes_table attributes(get_self(), serial);
    auto a = attributes.find(attribute_name.value);

    //authenticate
    require_auth(s.manager);

    //validate
    check(a != attributes.end(), "attribute not found");

    //erase attribute
    attributes.erase(a);

}