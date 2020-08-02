#include "../include/marble.hpp"

//======================== admin actions ========================

ACTION marble::init(string contract_name, string contract_version, name initial_admin) {
    
    //authenticate
    require_auth(get_self());

    //open config table
    config_table configs(get_self(), get_self().value);

    //validate
    check(!configs.exists(), "config already initialized");
    check(is_account(initial_admin), "initial admin account doesn't exist");

    //initialize
    config new_conf = {
        contract_name, //contract_name
        contract_version, //contract_version
        initial_admin, //admin
        uint64_t(0) //last_serial
    };

    //set new config
    configs.set(new_conf, get_self());

}

ACTION marble::setversion(string new_version) {
    
    //get config
    config_table configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //set new contract version
    conf.contract_version = new_version;

    //update configs table
    configs.set(conf, get_self());

}

ACTION marble::setadmin(name new_admin) {
    
    //open config table, get config
    config_table configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //validate
    check(is_account(new_admin), "new admin account doesn't exist");

    //set new admin
    conf.admin = new_admin;

    //update config table
    configs.set(conf, get_self());

}

//======================== group actions ========================

ACTION marble::newgroup(string title, string description, name group_name, name manager, uint64_t supply_cap) {
    
    //open config table, get config
    config_table configs(get_self(), get_self().value);
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

    //emplace new group
    //ram payer: self
    groups.emplace(get_self(), [&](auto& col) {
        col.title = title;
        col.description = description;
        col.group_name = group_name;
        col.manager = manager;
        col.supply = 0;
        col.issued_supply = 0;
        col.supply_cap = supply_cap;
    });

    //initialize
    map<name, bool> initial_behaviors;
    initial_behaviors["mint"_n] = true;
    initial_behaviors["transfer"_n] = true;
    initial_behaviors["activate"_n] = false;
    initial_behaviors["consume"_n] = false;
    initial_behaviors["destroy"_n] = true;

    //for each initial behavior, emplace new behavior
    for (pair p : initial_behaviors) {

        //open behaviors table, search for behavior
        behaviors_table behaviors(get_self(), group_name.value);
        auto bhvr_itr = behaviors.find(p.first.value);

        //validate
        check(bhvr_itr == behaviors.end(), "behavior already exists");

        //emplace new behavior
        //ram payer: contract
        behaviors.emplace(get_self(), [&](auto& col) {
            col.behavior_name = p.first;
            col.state = p.second;
            col.locked = false;
        });

    }

}

ACTION marble::editgroup(name group_name, string new_title, string new_description) {

    //get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(group_name.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //modify group
    groups.modify(grp, same_payer, [&](auto& col) {
        col.title = new_title;
        col.description = new_description;
    });

}

ACTION marble::setmanager(name group_name, name new_manager, string memo) {
    
    //get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(group_name.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //validate
    check(is_account(new_manager), "new manager account doesn't exist");

    //modify group
    groups.modify(grp, same_payer, [&](auto& col) {
        col.manager = new_manager;
    });

}

//======================== behavior actions ========================

ACTION marble::addbehavior(name group_name, name behavior_name, bool initial_state) {
    
    //get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(group_name.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //search for behavior
    behaviors_table behaviors(get_self(), group_name.value);
    auto bhvr_itr = behaviors.find(behavior_name.value);

    //validate
    check(bhvr_itr == behaviors.end(), "behavior already exists");

    //emplace new behavior
    //ram payer: self
    behaviors.emplace(get_self(), [&](auto& col) {
        col.behavior_name = behavior_name;
        col.state = initial_state;
        col.locked = false;
    });

}

ACTION marble::toggle(name group_name, name behavior_name) {
    
    //get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(group_name.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //get behavior
    behaviors_table behaviors(get_self(), group_name.value);
    auto& bhvr = behaviors.get(behavior_name.value, "behavior not found");

    //validate
    check(!bhvr.locked, "behavior is locked");

    //modify behavior
    behaviors.modify(bhvr, same_payer, [&](auto& col) {
        col.state = !bhvr.state;
    });

}

ACTION marble::lockbhvr(name group_name, name behavior_name) {

    //get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(group_name.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //get behavior
    behaviors_table behaviors(get_self(), group_name.value);
    auto& bhvr = behaviors.get(behavior_name.value, "behavior not found");

    //validate
    check(!bhvr.locked, "behavior already locked");

    //modify behavior
    behaviors.modify(bhvr, same_payer, [&](auto& col) {
        col.locked = true;
    });

}

ACTION marble::rmvbehavior(name group_name, name behavior_name) {
    
    //get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(group_name.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //get behavior
    behaviors_table behaviors(get_self(), group_name.value);
    auto& bhvr = behaviors.get(behavior_name.value, "behavior not found");

    //erase behavior
    behaviors.erase(bhvr);

}

//======================== item actions ========================

ACTION marble::mintitem(name to, name group_name) {
    
    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(group_name.value, "group name not found");

    //authenticate
    // require_auth(grp.manager);
    check(has_auth(grp.manager) || has_auth(get_self()), "only contract or group manager can mint items");

    //open behaviors table, get behavior
    behaviors_table behaviors(get_self(), group_name.value);
    auto& bhvr = behaviors.get(name("mint").value, "behavior not found");

    //validate
    check(bhvr.state, "item is not mintable");

    //validate
    check(is_account(to), "to account doesn't exist");
    check(grp.supply < grp.supply_cap, "supply cap reached");

    //open config table, get configs
    config_table configs(get_self(), get_self().value);
    auto conf = configs.get();

    //initialize
    auto now = time_point_sec(current_time_point());
    uint64_t new_serial = conf.last_serial + 1;
    string logevent_memo = "serial: " + to_string(new_serial);

    //increment last_serial, set new config
    conf.last_serial += 1;
    configs.set(conf, get_self());

    //open items table, find item
    items_table items(get_self(), get_self().value);
    auto itm = items.find(new_serial);

    //validate
    check(itm == items.end(), "serial already exists");

    //emplace new item
    //ram payer: self
    items.emplace(get_self(), [&](auto& col) {
        col.serial = new_serial;
        col.group = group_name;
        col.owner = to;
    });

    //update group
    groups.modify(grp, same_payer, [&](auto& col) {
        col.supply += 1;
        col.issued_supply += 1;
    });

    //inline logevent
    action(permission_level{get_self(), name("active")}, get_self(), name("logevent"), make_tuple(
        "mint"_n, //event_name
        int64_t(new_serial), //event_value
        now, //event_time
        logevent_memo //memo
    )).send();

}

ACTION marble::transferitem(name from, name to, vector<uint64_t> serials, string memo) {

    //validate
    check(is_account(to), "to account doesn't exist");

    //loop over serials
    for (uint64_t s : serials) {

        //open items table, get item
        items_table items(get_self(), get_self().value);
        auto& itm = items.get(s, "item not found");

        //authenticate
        require_auth(itm.owner);

        //open groups table, get group
        groups_table groups(get_self(), get_self().value);
        auto& grp = groups.get(itm.group.value, "group not found");

        //open behaviors table, get behavior
        behaviors_table behaviors(get_self(), itm.group.value);
        auto& bhvr = behaviors.get(name("transfer").value, "behavior not found");

        //validate
        check(bhvr.state, "item is not transferable");

        //update item
        items.modify(itm, same_payer, [&](auto& col) {
            col.owner = to;
        });

    }

    //notify from and to accounts
    require_recipient(from);
    require_recipient(to);

}

ACTION marble::activateitem(uint64_t serial) {
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //authenticate
    require_auth(itm.owner);

    //open behaviors table, get behavior
    behaviors_table behaviors(get_self(), itm.group.value);
    auto& bhvr = behaviors.get(name("activate").value, "behavior not found");

    //validate
    check(bhvr.state, "item is not activatable");

    //if trigger exists
    if (has_trigger(serial, name("activate"))) {
        //send inline exectrigger
        action(permission_level{get_self(), name("active")}, get_self(), name("exectrigger"), make_tuple(
            serial, //serial
            name("activate") //behavior_name
        )).send();
    }
}

ACTION marble::consumeitem(uint64_t serial) {
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //authenticate
    require_auth(itm.owner);

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //open behaviors table, get behavior
    behaviors_table behaviors(get_self(), itm.group.value);
    auto& bhvr = behaviors.get(name("consume").value, "behavior not found");

    //validate
    check(bhvr.state, "item is not consumable");
    check(grp.supply > 0, "cannot reduce supply below zero");

    //update group
    groups.modify(grp, same_payer, [&](auto& col) {
        col.supply -= 1;
    });

    //TODO: inject owner in backings since item won't exist during inline calls

    //erase item
    items.erase(itm);

    //if consume trigger exists
    if (has_trigger(serial, name("activate"))) {
        //send inline exectrigger
        action(permission_level{get_self(), name("active")}, get_self(), name("exectrigger"), make_tuple(
            serial, //serial
            name("consume") //behavior_name
        )).send();
    }
}

ACTION marble::destroyitem(uint64_t serial, string memo) {

    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open behaviors table, get behavior
    behaviors_table behaviors(get_self(), itm.group.value);
    auto& bhvr = behaviors.get(name("destroy").value, "behavior not found");

    //validate
    check(bhvr.state, "item is not destroyable");
    check(grp.supply > 0, "cannot reduce supply below zero");

    //update group
    groups.modify(grp, same_payer, [&](auto& col) {
        col.supply -= 1;
    });

    //erase item
    items.erase(itm);

}

//======================== tag actions ========================

ACTION marble::newtag(uint64_t serial, name tag_name, string content, optional<string> checksum, optional<string> algorithm) {

    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open tags table, search for tag
    tags_table tags(get_self(), serial);
    auto tg_itr = tags.find(tag_name.value);

    //validate
    check(tag_name != name(0), "tag name cannot be empty");
    check(tg_itr == tags.end(), "tag name already exists on item");

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
    //ram payer: self
    tags.emplace(get_self(), [&](auto& col) {
        col.tag_name = tag_name;
        col.content = content;
        col.checksum = chsum;
        col.algorithm = algo;
        col.locked = false;
    });

}

ACTION marble::updatetag(uint64_t serial, name tag_name, string new_content, optional<string> new_checksum, optional<string> new_algorithm) {

    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open tags table, get tag
    tags_table tags(get_self(), serial);
    auto& tg = tags.get(tag_name.value, "tag not found on item");

    //validate
    check(!tg.locked, "tag is locked");

    string new_chsum = "";
    string new_algo = tg.algorithm;

    if (new_checksum) {
        new_chsum = *new_checksum;
    }

    if (new_algorithm) {
        new_algo = *new_algorithm;
    }

    //update tag
    tags.modify(tg, same_payer, [&](auto& col) {
        col.content = new_content;
        col.checksum = new_chsum;
        col.algorithm = new_algo;
    });

}

ACTION marble::locktag(uint64_t serial, name tag_name) {

    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open tags table, get tag
    tags_table tags(get_self(), serial);
    auto& tg = tags.get(tag_name.value, "tag not found on item");

    //validate
    check(!tg.locked, "tag is already locked");

    //modify tag
    tags.modify(tg, same_payer, [&](auto& col) {
        col.locked = true;
    });

}

ACTION marble::rmvtag(uint64_t serial, name tag_name, string memo) {

    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open tags table, get tag
    tags_table tags(get_self(), serial);
    auto& tg = tags.get(tag_name.value, "tag not found on item");

    //erase item
    tags.erase(tg);

}

//======================== attribute actions ========================

ACTION marble::newattribute(uint64_t serial, name attribute_name, int64_t initial_points) {

    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open attributes table, get attribute
    attributes_table attributes(get_self(), serial);
    auto attr_itr = attributes.find(attribute_name.value);

    //validate
    check(attr_itr == attributes.end(), "attribute name already exists for item");

    //emplace new attribute
    //ram payer: self
    attributes.emplace(get_self(), [&](auto& col) {
        col.attribute_name = attribute_name;
        col.points = initial_points;
        col.locked = false;
    });

}

ACTION marble::setpoints(uint64_t serial, name attribute_name, int64_t new_points) {
    
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //open attributes table, get attribute
    attributes_table attributes(get_self(), serial);
    auto& attr = attributes.get(attribute_name.value, "attribute not found");

    //authenticate
    require_auth(grp.manager);

    //validate
    check(!attr.locked, "attribute is locked");

    //set new attribute points
    attributes.modify(attr, same_payer, [&](auto& col) {
        col.points = new_points;
    });

}

ACTION marble::increasepts(uint64_t serial, name attribute_name, uint64_t points_to_add) {
    
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //open attributes table, get attribute
    attributes_table attributes(get_self(), serial);
    auto& attr = attributes.get(attribute_name.value, "attribute not found");

    //authenticate
    require_auth(grp.manager);

    //validate
    check(!attr.locked, "attribute is locked");
    check(points_to_add > 0, "must add greater than zero points");

    //modify attribute points
    attributes.modify(attr, same_payer, [&](auto& col) {
        col.points += points_to_add;
    });

}

ACTION marble::decreasepts(uint64_t serial, name attribute_name, uint64_t points_to_subtract) {

    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open attributes table, get attribute
    attributes_table attributes(get_self(), serial);
    auto& attr = attributes.get(attribute_name.value, "attribute not found");

    //validate
    check(!attr.locked, "attribute is locked");
    check(points_to_subtract > 0, "must subtract greater than zero points");

    //modify attribute points
    attributes.modify(attr, same_payer, [&](auto& col) {
        col.points -= points_to_subtract;
    });

}

ACTION marble::lockattr(uint64_t serial, name attribute_name) {

    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open attributes table, get attribute
    attributes_table attributes(get_self(), serial);
    auto& attr = attributes.get(attribute_name.value, "attribute not found");

    //validate
    check(!attr.locked, "attribute is already locked");

    //modify attribute
    attributes.modify(attr, same_payer, [&](auto& col) {
        col.locked = true;
    });

}

ACTION marble::rmvattribute(uint64_t serial, name attribute_name) {

    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open attributes table, get attribute
    attributes_table attributes(get_self(), serial);
    auto attr = attributes.find(attribute_name.value);

    //validate
    check(attr != attributes.end(), "attribute not found");

    //erase attribute
    attributes.erase(attr);

}

//======================== event actions ========================

ACTION marble::logevent(name event_name, int64_t event_value, time_point_sec event_time, string memo) {

    //authenticate
    require_auth(get_self());

}

ACTION marble::newevent(uint64_t serial, name event_name, optional<time_point_sec> custom_event_time) {

    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open events table, search for event
    events_table events(get_self(), serial);
    auto evnt_itr = events.find(event_name.value);

    //validate
    check(evnt_itr == events.end(), "event already exists");

    //initialize
    time_point_sec now = time_point_sec(current_time_point());
    time_point_sec new_event_time = now;

    //if custom_event_time given
    if (custom_event_time) {
        new_event_time = *custom_event_time;
    }

    //emplace new event
    //ram payer: self
    events.emplace(get_self(), [&](auto& col) {
        col.event_name = event_name;
        col.event_time = new_event_time;
    });

}

ACTION marble::seteventtime(uint64_t serial, name event_name, time_point_sec new_event_time) {

    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open events table, get event
    events_table events(get_self(), serial);
    auto& evnt = events.get(event_name.value, "event not found");

    //validate
    check(!evnt.locked, "event is locked");

    //modify event
    events.modify(evnt, same_payer, [&](auto& col) {
        col.event_time += new_event_time;
    });

}

ACTION marble::lockevent(uint64_t serial, name event_name) {

    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open events table, get event
    events_table events(get_self(), serial);
    auto& evnt = events.get(event_name.value, "event not found");

    //validate
    check(!evnt.locked, "event is already locked");

    //modify event
    events.modify(evnt, same_payer, [&](auto& col) {
        col.locked = true;
    });

}

ACTION marble::rmvevent(uint64_t serial, name event_name) {

    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open events table, get event
    events_table events(get_self(), serial);
    auto& evnt = events.get(event_name.value, "event not found");

    //erase event
    events.erase(evnt);

}

//======================== backing actions ========================

ACTION marble::newbacking(uint64_t serial, asset amount, optional<name> release_auth, optional<asset> per_release) {
    //validate
    check(amount.symbol == CORE_SYM, "asset must be core symbol");

    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open accounts table, get manager account
    accounts_table accounts(get_self(), grp.manager.value);
    auto& acct = accounts.get(amount.symbol.code().raw(), "manager account not found");

    //validate
    check(acct.balance >= amount, "insufficient funds");
    check(amount.amount > 0, "must back with a positive amount");

    //subtract from account balance
    accounts.modify(acct, same_payer, [&](auto& col) {
        col.balance -= amount;
    });

    //open backings table, search for backing
    backings_table backings(get_self(), serial);
    auto back_itr = backings.find(amount.symbol.code().raw());

    //initialize
    name backing_release_auth = get_self();
    asset backing_per_release = amount;

    //if release_auth param provided
    if (release_auth) {
        backing_release_auth = *release_auth;
    }

    //if per_release param provided
    if (per_release) {
        backing_per_release = *per_release;
    }

    //if backing not found
    if (back_itr == backings.end()) {
        //emplace new backing
        //ram payer: contract
        backings.emplace(get_self(), [&](auto& col) {
            col.backing_amount = amount;
            col.release_auth = backing_release_auth;
            col.per_release = backing_per_release;
            col.locked = false;
        });
    } else {
        //add to existing backing
        backings.modify(*back_itr, same_payer, [&](auto& col) {
            col.backing_amount += amount;
        });
    }

}

ACTION marble::release(uint64_t serial, symbol token_symbol, name release_to) {
    //validate
    check(token_symbol == CORE_SYM, "only core symbol allowed");

    //open backings table, get backing
    backings_table backings(get_self(), serial);
    auto& back = backings.get(token_symbol.code().raw(), "backing not found");

    //authenticate
    require_auth(back.release_auth);

    //validate
    check(back.backing_amount.amount >= back.per_release.amount, "cannot release more than backing amount");

    //open accounts table, search for account
    accounts_table accounts(get_self(), release_to.value);
    auto acct_itr = accounts.find(token_symbol.code().raw());

    //if account found
    if (acct_itr != accounts.end()) {
        //add to existing account
        accounts.modify(*acct_itr, same_payer, [&](auto& col) {
            col.balance += back.per_release;
        });
    } else {
        //create new account
        //ram payer: contract
        accounts.emplace(get_self(), [&](auto& col) {
            col.balance = back.per_release;
        });
    }

    //if backing amount greater than per release amount
    if (back.backing_amount.amount > back.per_release.amount) {
        //subtract release amount from backing
        backings.modify(back, same_payer, [&](auto& col) {
            col.backing_amount -= back.per_release;
        });
    } else {
        //erase backing
        backings.erase(back);
    }

}

//======================== trigger actions ========================

ACTION marble::newtrigger(uint64_t serial, name behavior_name, vector<char> trx_payload, optional<uint16_t> total_execs) {
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open triggers table, search for trigger
    triggers_table triggers(get_self(), serial);
    auto trig_itr = triggers.find(behavior_name.value);

    //validate
    check(trig_itr == triggers.end(), "trigger already exists");

    //initialize
    transaction_header trx_header;
    vector<action> context_free_actions;
    vector<action> actions;
    datastream<const char*> ds(trx_payload.data(), trx_payload.size());

    //read from datastream
    ds >> trx_header;
    ds >> context_free_actions;
    ds >> actions;

    //validate
    check(trx_header.expiration >= time_point_sec(current_time_point()), "transaction expired");
    check(context_free_actions.empty(), "not allowed to exec a trigger with context-free actions");

    //validate transaction
    for (action act : actions) {
        check(act.account == get_self(), "only transactions to self allowed");
        check(allowed_trigger_action(act.name), "action not allowed in trigger");
        
        // release_params params = act.data_as<release_params>();
        
        // // string msg = act.name.to_string() + " >>> " + string(act.data.begin(), act.data.end());
        // string msg = act.name.to_string() + " >>> " 
        //     + to_string(params.r_serial) + " " 
        //     + to_string(params.r_sym.precision()) + ","
        //     + params.r_sym.code().to_string() + " " 
        //     + params.r_to.to_string();

        // check(false, msg);
    }

    //initialize
    uint16_t remaining_execs = 1;

    //if total execs provided
    if (total_execs) {
        remaining_execs = *total_execs;
    } 

    //emplace new trigger
    //ram payer: contract
    triggers.emplace(get_self(), [&](auto& col) {
        col.behavior_name = behavior_name;
        col.trx_payload = trx_payload;
        col.remaining_execs = remaining_execs;
        col.primed = true;
        col.auto_prime = true;
        col.auto_erase = true;
    });

}

ACTION marble::setpayload(uint64_t serial, name behavior_name, vector<char> new_payload) {
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");
    
    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");
    
    //authenticate
    require_auth(grp.manager);

    //open triggers table, get trigger
    triggers_table triggers(get_self(), serial);
    auto& trig = triggers.get(behavior_name.value, "trigger not found");

    //validate
    check(!trig.locked, "trigger is locked");

    //update trigger payload
    triggers.modify(trig, same_payer, [&](auto& col) {
        col.trx_payload = new_payload;
    });
}

ACTION marble::exectrigger(uint64_t serial, name behavior_name) {
    //authenticate
    require_auth(get_self());
    
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open triggers table, get trigger
    triggers_table triggers(get_self(), serial);
    auto& trig = triggers.get(behavior_name.value, "trigger not found");

    //validate
    check(trig.remaining_execs > 0, "no remaining trigger executions");
    check(trig.primed, "trigger not primed");

    //TODO: compare trigger condition

    //initialize
    transaction_header trx_header;
    vector<action> context_free_actions;
    vector<action> actions;
    datastream<const char*> ds(trig.trx_payload.data(), trig.trx_payload.size());

    //read from datastream
    ds >> trx_header;
    ds >> context_free_actions;
    ds >> actions;

    //validate
    check(trx_header.expiration >= time_point_sec(current_time_point()), "transaction expired");
    check(context_free_actions.empty(), "not allowed to exec a trigger with context-free actions");

    //authenticate
    // check(would_authenticate());

    //if last remaining exec and auto_erase is true
    if (trig.remaining_execs == 1 && trig.auto_erase) {
        //erase trigger
        triggers.erase(trig);
    } else {
        //update trigger
        triggers.modify(trig, same_payer, [&](auto& col) {
            col.remaining_execs -= 1;
            col.primed = trig.auto_prime;
        });
    }

    //send all actions in trx as inlines
    for (const auto& act : actions) {
        act.send();
    }

}

//======================== frame actions ========================

ACTION marble::newframe(name frame_name, name group, map<name, string> default_tags, map<name, int64_t> default_attributes) {

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open frames table, find frame
    frames_table frames(get_self(), get_self().value);
    auto frm_itr = frames.find(frame_name.value);

    //validate
    check(frm_itr == frames.end(), "frame already exists");

    //emplace new frame
    //ram payer: self
    frames.emplace(get_self(), [&](auto& col) {
        col.frame_name = frame_name;
        col.group = group;
        col.default_tags = default_tags;
        col.default_attributes = default_attributes;
    });

}

ACTION marble::applyframe(name frame_name, uint64_t serial, bool overwrite) {

    //open frames table, get frame
    frames_table frames(get_self(), get_self().value);
    auto& frm = frames.get(frame_name.value, "frame not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(frm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //apply default tags
    for (auto itr = frm.default_tags.begin(); itr != frm.default_tags.end(); itr++) {

        //open tags table, find tag
        tags_table tags(get_self(), serial);
        auto tg_itr = tags.find(itr->first.value);

        //NOTE: will skip existing tag with same tag name if overwrite is false

        //if tag not found
        if (tg_itr == tags.end()) {
            
            //emplace new tag
            //ram payer: self
            tags.emplace(get_self(), [&](auto& col) {
                col.tag_name = itr->first;
                col.content = itr->second;
                col.checksum = "";
                col.algorithm = "";
            });

        } else if (overwrite) {

            //validate
            check(!tg_itr->locked, "tag is locked");

            //overwrite existing tag
            tags.modify(tg_itr, same_payer, [&](auto& col) {
                col.content = itr->second;
                col.checksum = "";
                col.algorithm = "";
            });

        }

    }

    //apply default attributes
    for (auto itr = frm.default_attributes.begin(); itr != frm.default_attributes.end(); itr++) {

        //open attributes table, find attribute
        attributes_table attributes(get_self(), serial);
        auto attr_itr = attributes.find(itr->first.value);

        //NOTE: will skip existing attribute with same attribute name if overwrite is false

        //if attribute not found
        if (attr_itr == attributes.end()) {
            
            //emplace new attribute
            //ram payer: self
            attributes.emplace(get_self(), [&](auto& col) {
                col.attribute_name = itr->first;
                col.points = itr->second;
            });

        } else if (overwrite) {

            //validate
            check(!attr_itr->locked, "attribute is locked");

            //overwrite existing attribute
            attributes.modify(attr_itr, same_payer, [&](auto& col) {
                col.points = itr->second;
            });

        }

    }

}

ACTION marble::quickbuild(name frame_name, name to, map<name, string> override_tags, map<name, int64_t> override_attributes) {

    //open frames table, get frame
    frames_table frames(get_self(), get_self().value);
    auto& frm = frames.get(frame_name.value, "frame not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(frm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //initialize
    uint64_t item_serial;

    //open config table, get config
    config_table configs(get_self(), get_self().value);
    auto conf = configs.get();

    //set new serial
    item_serial = conf.last_serial + 1;

    //queue inline mintitem()
    action(permission_level{get_self(), name("active")}, get_self(), name("mintitem"), make_tuple(
        to, //to
        frm.group //group_name
    )).send();

    //apply default tags
    for (auto itr = frm.default_tags.begin(); itr != frm.default_tags.end(); itr++) {

        //open tags table, search for tag
        tags_table tags(get_self(), item_serial);
        auto tg_itr = tags.find(itr->first.value);

        //if tag not found
        if (tg_itr == tags.end()) {
            
            //emplace new tag
            //ram payer: contract
            tags.emplace(get_self(), [&](auto& col) {
                col.tag_name = itr->first;
                col.content = itr->second;
                col.checksum = "";
                col.algorithm = "";
            });

        } else {

            //overwrite existing tag
            tags.modify(tg_itr, same_payer, [&](auto& col) {
                col.content = itr->second;
                col.checksum = "";
                col.algorithm = "";
            });

        }

    }

    //apply default attributes
    for (auto itr = frm.default_attributes.begin(); itr != frm.default_attributes.end(); itr++) {

        //open attributes table, find attribute
        attributes_table attributes(get_self(), item_serial);
        auto attr_itr = attributes.find(itr->first.value);

        //if attribute not found
        if (attr_itr == attributes.end()) {
            
            //emplace new attribute
            //ram payer: contract
            attributes.emplace(get_self(), [&](auto& col) {
                col.attribute_name = itr->first;
                col.points = itr->second;
            });

        } else {

            //overwrite existing attribute
            attributes.modify(attr_itr, same_payer, [&](auto& col) {
                col.points = itr->second;
            });

        }

    }

    //apply tag overrides
    for (auto itr = override_tags.begin(); itr != override_tags.end(); itr++) {

        //open tags table, search for tag
        tags_table tags(get_self(), item_serial);
        auto tg_itr = tags.find(itr->first.value);

        //if tag not found
        if (tg_itr == tags.end()) {
            
            //emplace new tag
            //ram payer: contract
            tags.emplace(get_self(), [&](auto& col) {
                col.tag_name = itr->first;
                col.content = itr->second;
                col.checksum = "";
                col.algorithm = "";
            });

        } else {

            //overwrite existing tag
            tags.modify(tg_itr, same_payer, [&](auto& col) {
                col.content = itr->second;
                col.checksum = "";
                col.algorithm = "";
            });

        }

    }

    //apply attribute overrides
    for (auto itr = override_attributes.begin(); itr != override_attributes.end(); itr++) {

        //open attributes table, find attribute
        attributes_table attributes(get_self(), item_serial);
        auto attr_itr = attributes.find(itr->first.value);

        //if attribute not found
        if (attr_itr == attributes.end()) {
            
            //emplace new attribute
            //ram payer: self
            attributes.emplace(get_self(), [&](auto& col) {
                col.attribute_name = itr->first;
                col.points = itr->second;
            });

        } else {

            //overwrite existing attribute
            attributes.modify(attr_itr, same_payer, [&](auto& col) {
                col.points = itr->second;
            });

        }

    }

}

ACTION marble::rmvframe(name frame_name, string memo) {

    //open frames table, get frame
    frames_table frames(get_self(), get_self().value);
    auto& frm = frames.get(frame_name.value, "frame not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(frm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //erase frame
    frames.erase(frm);

}

//======================== account actions ========================

ACTION marble::withdraw(name account_owner, asset amount) {
    //validate
    check(amount.symbol == CORE_SYM, "can only withdraw core token");
    
    //authenticate
    require_auth(account_owner);

    //open accounts table, get deposit
    accounts_table accounts(get_self(), account_owner.value);
    auto& acct = accounts.get(amount.symbol.code().raw(), "account not found");

    //validate
    check(amount.amount > 0, "must withdraw a positive amount");
    check(acct.balance >= amount, "insufficient funds");

    //update account balance
    accounts.modify(acct, same_payer, [&](auto& col) {
        col.balance -= amount;
    });

    //send inline eosio.token::transfer to withdrawing account
    //auth: self
    action(permission_level{get_self(), name("active")}, name("eosio.token"), name("transfer"), make_tuple(
        get_self(), //from
        account_owner, //to
        amount, //quantity
        std::string("Marble Account Withdrawal") //memo
    )).send();
}

//======================== notification handlers ========================

void marble::catch_transfer(name from, name to, asset quantity, string memo) {
    //get initial receiver contract
    name rec = get_first_receiver();

    //if received notification from eosio.token, not from self, and symbol is TLOS
    if (rec == name("eosio.token") && from != get_self() && quantity.symbol == CORE_SYM) {
        //if memo is "deposit"
        if (memo == std::string("deposit")) { 
            //open accounts table, search for account
            accounts_table accounts(get_self(), from.value);
            auto acct_itr = accounts.find(CORE_SYM.code().raw());

            //if account found
            if (acct_itr != accounts.end()) {
                //add to existing account
                accounts.modify(*acct_itr, same_payer, [&](auto& col) {
                    col.balance += quantity;
                });
            } else {
                //create new account
                //ram payer: contract
                accounts.emplace(get_self(), [&](auto& col) {
                    col.balance = quantity;
                });
            }
        }
    }
}

//======================== contract functions ========================

bool marble::has_trigger(uint64_t serial, name behavior_name) {
    //open triggers table, get trigger
    triggers_table triggers(get_self(), serial);
    auto trig_itr = triggers.find(behavior_name.value);

    //if trigger found
    if (trig_itr != triggers.end()) {
        return true;
    }

    return false;
}

bool marble::allowed_trigger_action(name action_name) {
    //return true if action is allowed in a trigger
    switch (action_name.value) {
        case (name("release").value):
            return true;
            break;
        default:
            return false;
            break;
    }
}
