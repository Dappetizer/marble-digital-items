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

ACTION marble::setaccess(name new_access, string memo) {
    //open configs singleton, get config
    configs_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //set new access
    conf.access = new_access;

    //update configs singleton
    configs.set(conf, get_self());
}

//======================== group actions ========================

ACTION marble::newcollectn(string title, string description, name collection_name, name manager, uint64_t supply_cap) {
    //open collections table, find collection
    collections_table collections(get_self(), get_self().value);
    auto s_itr = collections.find(collection_name.value);
    
    //authenticate
    require_auth(manager);

    //validate
    check(s_itr == collections.end(), "collection name already taken");
    check(supply_cap > 0, "supply cap must be greater than zero");

    //initialize
    map<name, bool> initial_options;
    initial_options["transferable"_n] = false;
    initial_options["destructible"_n] = false;
    initial_options["updateable"_n] = false;
    initial_options["increasable"_n] = false;
    initial_options["decreasable"_n] = false;

    //emplace new collection
    collections.emplace(get_self(), [&](auto& col) {
        col.collection_name = collection_name;
        col.manager = manager;
        col.title = title;
        col.description = description;
        col.supply = 0;
        col.issued_supply = 0;
        col.supply_cap = supply_cap;
        col.options = initial_options;
    });
}

ACTION marble::addoption(name collection_name, name option_name, bool initial_value) {
    //oen collections table, get collection
    collections_table collections(get_self(), get_self().value);
    auto& c = collections.get(collection_name.value, "collection not found");

    //authenticate
    require_auth(c.manager);

    //validate
    check(c.options.find(option_name) == c.options.end(), "option already exists");

    //initialize
    auto new_options = c.options;
    new_options[option_name] = initial_value;

    collections.modify(c, same_payer, [&](auto& col) {
        col.options = new_options;
    });
}

ACTION marble::toggle(name collection_name, name option_name, string memo) {
    //open collections table, get collection
    collections_table collections(get_self(), get_self().value);
    auto& c = collections.get(collection_name.value, "collection not found");

    //authenticate
    require_auth(c.manager);

    //validate
    check(c.options.find(option_name) != c.options.end(), "option not found");

    //initialize
    auto new_options = c.options;
    new_options[option_name] = !new_options.at(option_name);

    //update collection options
    collections.modify(c, same_payer, [&](auto& col) {
        col.options = new_options;
    });
}

ACTION marble::rmvoption(name collection_name, name option_name) {
    //open collections table, get collection
    collections_table collections(get_self(), get_self().value);
    auto& c = collections.get(collection_name.value, "collection not found");

    //authenticate
    require_auth(c.manager);

    auto opt_itr = c.options.find(option_name);

    //validate
    check(opt_itr != c.options.end(), "option not found");

    //update collection options
    collections.modify(c, same_payer, [&](auto& col) {
        col.options.erase(opt_itr);
    });
}

ACTION marble::setmanager(name collection_name, name new_manager, string memo) {
    //open collections table, get collection
    collections_table collections(get_self(), get_self().value);
    auto& c = collections.get(collection_name.value, "collection not found");

    //authenticate
    require_auth(c.manager);

    //validate
    check(is_account(new_manager), "new manager account doesn't exist");

    //update collection
    collections.modify(c, same_payer, [&](auto& col) {
        col.manager = new_manager;
    });
}

//======================== nft actions ========================

ACTION marble::newnft(name owner, name collection_name, string content, 
    optional<string> checksum, optional<string> algorithm) {
    //open collections table, get collection
    collections_table collections(get_self(), get_self().value);
    auto& c = collections.get(collection_name.value, "collection name not found");

    //authenticate
    require_auth(c.manager);

    //validate
    check(c.supply + 1 <= c.supply_cap, "supply cap reached");

    string initial_checksum = "";
    string initial_algo = "";

    if (checksum) {
        initial_checksum = *checksum;
    }

    if (algorithm) {
        initial_algo = *algorithm;
    }

    //open configs singleton, get configs
    configs_singleton configs(get_self(), get_self().value);
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
        col.collection = collection_name;
        col.owner = owner;
        col.content = content;
        col.checksum = initial_checksum;
        col.algorithm = initial_algo;
    });

    //update collection
    collections.modify(c, same_payer, [&](auto& col) {
        col.supply += 1;
        col.issued_supply += 1;
    });
}

ACTION marble::updatenft(uint64_t serial, string content, 
    optional<string> checksum, optional<string> algorithm) {

    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    collections_table collections(get_self(), get_self().value);
    auto& c = collections.get(n.collection.value, "collection not found");

    //authenticate
    require_auth(c.manager);

    //validate
    check(c.options.at("updateable"_n) == true, "nft not updateable");

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

ACTION marble::transfernft(uint64_t serial, name new_owner, string memo) {

    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    collections_table collections(get_self(), get_self().value);
    auto& c = collections.get(n.collection.value, "collection not found");

    //authenticate
    require_auth(c.manager);

    //validate
    check(is_account(new_owner), "new owner account doesn't exist");
    check(c.options.at("transferable"_n) == true, "nft is not transferable");

    //update nft
    nfts.modify(n, same_payer, [&](auto& col) {
        col.owner = new_owner;
    });

    //notify new owner
    require_recipient(new_owner);

}

ACTION marble::destroynft(uint64_t serial, string memo) {

    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    collections_table collections(get_self(), get_self().value);
    auto& c = collections.get(n.collection.value, "collection not found");

    //authenticate
    require_auth(c.manager);

    //validate
    check(c.options.at("destructible"_n) == true, "nft is not destructible");
    check(c.supply > 0, "cannot reduce below zero supply");

    //update collection
    collections.modify(c, same_payer, [&](auto& col) {
        col.supply -= 1;
    });

    //erase nft
    nfts.erase(n);

}

//======================== attribute actions ========================

ACTION marble::newattribute(uint64_t serial, name attribute_name, uint64_t initial_points) {

    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    collections_table collections(get_self(), get_self().value);
    auto& c = collections.get(n.collection.value, "collection not found");

    attributes_table attributes(get_self(), serial);
    auto a = attributes.find(attribute_name.value);

    //authenticate
    require_auth(c.manager);

    //validate
    check(initial_points >= 0, "initial points cannot be negative");
    check(a == attributes.end(), "attribute name already exists for nft");

    attributes.emplace(c.manager, [&](auto& col) {
        col.attribute_name = attribute_name;
        col.points = initial_points;
    });

}

ACTION marble::setpoints(uint64_t serial, name attribute_name, uint64_t new_points) {
    //open nfts table, get nft
    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    collections_table collections(get_self(), get_self().value);
    auto& c = collections.get(n.collection.value, "collection not found");

    attributes_table attributes(get_self(), serial);
    auto& a = attributes.get(attribute_name.value, "attribute not found");

    //authenticate
    require_auth(c.manager);

    //validate
    check(new_points > 0, "new points greater than zero");

    attributes.modify(a, same_payer, [&](auto& col) {
        col.points = new_points;
    });
}

ACTION marble::increasepts(uint64_t serial, name attribute_name, uint64_t points_to_add) {
    //open nfts table, get nft
    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    collections_table collections(get_self(), get_self().value);
    auto& c = collections.get(n.collection.value, "collection not found");

    attributes_table attributes(get_self(), serial);
    auto& a = attributes.get(attribute_name.value, "attribute not found");

    //authenticate
    require_auth(c.manager);

    //validate
    check(c.options.at("increasable"_n), "nft not increasable");
    check(points_to_add > 0, "must add greater than zero points");

    attributes.modify(a, same_payer, [&](auto& col) {
        col.points += points_to_add;
    });
}

ACTION marble::decreasepts(uint64_t serial, name attribute_name, uint64_t points_to_subtract) {

    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    collections_table collections(get_self(), get_self().value);
    auto& c = collections.get(n.collection.value, "collection not found");

    attributes_table attributes(get_self(), serial);
    auto& a = attributes.get(attribute_name.value, "attribute not found");

    //authenticate
    require_auth(c.manager);

    //validate
    check(c.options.at("decreasable"_n), "nft not decreasable");
    check(points_to_subtract > 0, "must subtract greater than zero points");
    check(points_to_subtract <= a.points, "cannot subtract points below zero");

    attributes.modify(a, same_payer, [&](auto& col) {
        col.points -= points_to_subtract;
    });

}

ACTION marble::rmvattribute(uint64_t serial, name attribute_name) {

    nfts_table nfts(get_self(), get_self().value);
    auto& n = nfts.get(serial, "nft not found");

    collections_table collections(get_self(), get_self().value);
    auto& c = collections.get(n.collection.value, "collection not found");

    attributes_table attributes(get_self(), serial);
    auto a = attributes.find(attribute_name.value);

    //authenticate
    require_auth(c.manager);

    //validate
    check(a != attributes.end(), "attribute not found");

    //erase attribute
    attributes.erase(a);

}