#include "../include/marble.hpp"

//======================== config actions ========================

ACTION marble::init(string contract_name, string contract_version, name initial_admin)
{
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

ACTION marble::setversion(string new_version)
{
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

ACTION marble::setadmin(name new_admin)
{
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

ACTION marble::newgroup(string title, string description, name group_name, name manager, uint64_t supply_cap)
{
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
    initial_behaviors["reclaim"_n] = false;
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

ACTION marble::editgroup(name group_name, string new_title, string new_description)
{
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

ACTION marble::setmanager(name group_name, name new_manager, string memo)
{
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

ACTION marble::addbehavior(name group_name, name behavior_name, bool initial_state)
{
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

ACTION marble::togglebhvr(name group_name, name behavior_name)
{
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

ACTION marble::lockbhvr(name group_name, name behavior_name)
{
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

ACTION marble::rmvbehavior(name group_name, name behavior_name)
{
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

ACTION marble::mintitem(name to, name group_name)
{
    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(group_name.value, "group name not found");

    //authenticate
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
        logevent_memo, //memo
        false //shared
    )).send();
}

ACTION marble::transferitem(name from, name to, vector<uint64_t> serials, string memo)
{
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

ACTION marble::activateitem(uint64_t serial)
{
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
}

ACTION marble::reclaimitem(uint64_t serial)
{
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
    auto& bhvr = behaviors.get(name("reclaim").value, "behavior not found");

    //validate
    check(bhvr.state, "item is not reclaimable");

    //update item
    items.modify(itm, same_payer, [&](auto& col) {
        col.owner = grp.manager;
    });
}

ACTION marble::consumeitem(uint64_t serial)
{
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

    //open bonds table, find bond
    bonds_table bonds(get_self(), serial);
    auto bond_itr = bonds.find(CORE_SYM.code().raw());

    //if bond found
    if (bond_itr != bonds.end()) {
        //send inline marble::releaseall to self
        //auth: self
        action(permission_level{get_self(), name("active")}, get_self(), name("releaseall"), make_tuple(
            serial, //serial
            itm.owner //release_to
        )).send();
    }

    //update group
    groups.modify(grp, same_payer, [&](auto& col) {
        col.supply -= 1;
    });

    //erase item
    items.erase(itm);
}

ACTION marble::destroyitem(uint64_t serial, string memo)
{
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

    //open bonds table, find bond
    bonds_table bonds(get_self(), serial);
    auto bond_itr = bonds.find(CORE_SYM.code().raw());

    //if bond found
    if (bond_itr != bonds.end()) {
        //send inline marble::releaseall to self
        //auth: self
        action(permission_level{get_self(), name("active")}, get_self(), name("releaseall"), make_tuple(
            serial, //serial
            itm.owner //release_to
        )).send();
    }

    //update group
    groups.modify(grp, same_payer, [&](auto& col) {
        col.supply -= 1;
    });

    //erase item
    items.erase(itm);
}

//======================== tag actions ========================

ACTION marble::newtag(uint64_t serial, name tag_name, string content, optional<string> checksum, optional<string> algorithm, bool shared)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //validate
    check(tag_name != name(0), "tag name cannot be empty");

    //initialize
    string chsum = (checksum) ? *checksum : "";
    string algo = (algorithm) ? *algorithm : "";

    //if shared tag
    if (shared) {
        //open shared tags table, find shared tag
        shared_tags_table shared_tags(get_self(), grp.group_name.value);
        auto shared_tag_itr = shared_tags.find(tag_name.value);

        //validate
        check(shared_tag_itr == shared_tags.end(), "shared tag name already exists");

        //emplace shared tag
        //ram payer: self
        shared_tags.emplace(get_self(), [&](auto& col) {
            col.tag_name = tag_name;
            col.content = content;
            col.checksum = chsum;
            col.algorithm = algo;
            col.locked = false;
        });
    } else {
        //open tags table, find tag
        tags_table tags(get_self(), serial);
        auto tag_itr = tags.find(tag_name.value);

        //validate
        check(tag_itr == tags.end(), "tag name already exists on item");

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
}

ACTION marble::updatetag(uint64_t serial, name tag_name, string new_content, optional<string> new_checksum, optional<string> new_algorithm, bool shared)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    string new_chsum = (new_checksum) ? *new_checksum : "";
    string new_algo = (new_algorithm) ? *new_algorithm : "";

    // if (new_checksum) {
    //     new_chsum = *new_checksum;
    // }

    // if (new_algorithm) {
    //     new_algo = *new_algorithm;
    // }

    //if shared tag
    if (shared) {
        //open shared tags table, get shared tag
        shared_tags_table shared_tags(get_self(), grp.group_name.value);
        auto& st = shared_tags.get(tag_name.value, "shared tag not found");

        //validate
        check(!st.locked, "shared tag is locked");

        //update shared tag
        shared_tags.modify(st, same_payer, [&](auto& col) {
            col.content = new_content;
            col.checksum = new_chsum;
            col.algorithm = new_algo;
        });
    } else {
        //open tags table, get tag
        tags_table tags(get_self(), serial);
        auto& t = tags.get(tag_name.value, "tag not found on item");

        //validate
        check(!t.locked, "tag is locked");

        //update tag
        tags.modify(t, same_payer, [&](auto& col) {
            col.content = new_content;
            col.checksum = new_chsum;
            col.algorithm = new_algo;
        });
    }
}

ACTION marble::locktag(uint64_t serial, name tag_name, bool shared)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //if shared tag
    if (shared) {
        //open shared tags table, get shared tag
        shared_tags_table shared_tags(get_self(), grp.group_name.value);
        auto& st = shared_tags.get(tag_name.value, "shared tag not found");

        //validate
        check(!st.locked, "shared tag is already locked");

        //modify shared tag
        shared_tags.modify(st, same_payer, [&](auto& col) {
            col.locked = true;
        });
    } else {
        //open tags table, get tag
        tags_table tags(get_self(), serial);
        auto& t = tags.get(tag_name.value, "tag not found on item");

        //validate
        check(!t.locked, "tag is already locked");

        //modify tag
        tags.modify(t, same_payer, [&](auto& col) {
            col.locked = true;
        });
    }
}

ACTION marble::rmvtag(uint64_t serial, name tag_name, string memo, bool shared)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //if shared tag
    if (shared) {
        //open shared tags table, get shared tag
        shared_tags_table shared_tags(get_self(), grp.group_name.value);
        auto& st = shared_tags.get(tag_name.value, "shared tag not found");

        //remove tag
        shared_tags.erase(st);
    } else {
        //open tags table, get tag
        tags_table tags(get_self(), serial);
        auto& t = tags.get(tag_name.value, "tag not found on item");

        //remove item
        tags.erase(t);
    }
}

//======================== attribute actions ========================

ACTION marble::newattribute(uint64_t serial, name attribute_name, int64_t initial_points, bool shared)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //if shared attribute
    if (shared) {
        //open shared attributes table, find shared attribute
        shared_attributes_table shared_attributes(get_self(), grp.group_name.value);
        auto sh_attr_itr = shared_attributes.find(attribute_name.value);

        //validate
        check(sh_attr_itr == shared_attributes.end(), "shared attributes already exists");

        //create new shared attribute
        //ram payer: contract
        shared_attributes.emplace(get_self(), [&](auto& col) {
            col.attribute_name = attribute_name;
            col.points = initial_points;
            col.locked = false;
        });
    } else {
        //open attributes table, find attribute
        attributes_table attributes(get_self(), serial);
        auto attr_itr = attributes.find(attribute_name.value);

        //validate
        check(attr_itr == attributes.end(), "attribute name already exists for item");

        //create new attribute
        //ram payer: contract
        attributes.emplace(get_self(), [&](auto& col) {
            col.attribute_name = attribute_name;
            col.points = initial_points;
            col.locked = false;
        });
    }
}

ACTION marble::setpoints(uint64_t serial, name attribute_name, int64_t new_points, bool shared)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //if shared attribute
    if (shared) {
        //open shared attributes table, get shared attribute
        shared_attributes_table shared_attributes(get_self(), grp.group_name.value);
        auto& sh_attr = shared_attributes.get(attribute_name.value, "shared attribute not found");

        //validate
        check(!sh_attr.locked, "shared attribute is locked");

        //update shared attribute
        shared_attributes.modify(sh_attr, same_payer, [&](auto& col) {
            col.points = new_points;
        });
    } else {
        //open attributes table, get attribute
        attributes_table attributes(get_self(), serial);
        auto& attr = attributes.get(attribute_name.value, "attribute not found");

        //validate
        check(!attr.locked, "attribute is locked");

        //update attribute
        attributes.modify(attr, same_payer, [&](auto& col) {
            col.points = new_points;
        });
    }
}

ACTION marble::increasepts(uint64_t serial, name attribute_name, uint64_t points_to_add, bool shared)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //validate
    check(points_to_add > 0, "points to add must be greater than zero");

    //if shared attribute
    if (shared) {
        //open shared attributes table, get shared attribute
        shared_attributes_table shared_attributes(get_self(), grp.group_name.value);
        auto& sh_attr = shared_attributes.get(attribute_name.value, "shared attribute not found");

        //validate
        check(!sh_attr.locked, "shared attribute is locked");
        
        //update shared attribute
        shared_attributes.modify(sh_attr, same_payer, [&](auto& col) {
            col.points += points_to_add;
        });
    } else {
        //open attributes table, get attribute
        attributes_table attributes(get_self(), serial);
        auto& attr = attributes.get(attribute_name.value, "attribute not found");

        //validate
        check(!attr.locked, "attribute is locked");

        //update attribute
        attributes.modify(attr, same_payer, [&](auto& col) {
            col.points += points_to_add;
        });
    }
}

ACTION marble::decreasepts(uint64_t serial, name attribute_name, uint64_t points_to_subtract, bool shared)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //validate
    check(points_to_subtract > 0, "must subtract greater than zero points");

    //if shared attribute
    if (shared) {
        //open shared attributes table, get shared attribute
        shared_attributes_table shared_attributes(get_self(), grp.group_name.value);
        auto& sh_attr = shared_attributes.get(attribute_name.value, "shared attribute not found");

        //validate
        check(!sh_attr.locked, "shared attribute is locked");

        //udpate shared attribute
        shared_attributes.modify(sh_attr, same_payer, [&](auto& col) {
            col.points -= points_to_subtract;
        });
    } else {
        //open attributes table, get attribute
        attributes_table attributes(get_self(), serial);
        auto& attr = attributes.get(attribute_name.value, "attribute not found");

        //validate
        check(!attr.locked, "attribute is locked");

        //update attribute
        attributes.modify(attr, same_payer, [&](auto& col) {
            col.points -= points_to_subtract;
        });
    }
}

ACTION marble::lockattr(uint64_t serial, name attribute_name, bool shared)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //if shared attribute
    if (shared) {
        //open shared attributes table, get shared attribute
        shared_attributes_table shared_attributes(get_self(), grp.group_name.value);
        auto& sh_attr = shared_attributes.get(attribute_name.value, "shared attribute not found");

        //validate
        check(!sh_attr.locked, "shared attribute is already locked");

        //update shared attribute
        shared_attributes.modify(sh_attr, same_payer, [&](auto& col) {
            col.locked = true;
        });
    } else {
        //open attributes table, get attribute
        attributes_table attributes(get_self(), serial);
        auto& attr = attributes.get(attribute_name.value, "attribute not found");

        //validate
        check(!attr.locked, "attribute is already locked");

        //update attribute
        attributes.modify(attr, same_payer, [&](auto& col) {
            col.locked = true;
        });
    }
}

ACTION marble::rmvattribute(uint64_t serial, name attribute_name, bool shared)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //if shared attribute
    if (shared) {
        //open shared attributes table, get shared attribute
        shared_attributes_table shared_attributes(get_self(), grp.group_name.value);
        auto& sh_attr = shared_attributes.get(attribute_name.value, "shared attribute not found");

        //remove shared attribute
        shared_attributes.erase(sh_attr);
    } else {
        //open attributes table, get attribute
        attributes_table attributes(get_self(), serial);
        auto& attr = attributes.get(attribute_name.value, "attribute not found");

        //remove attribute
        attributes.erase(attr);
    }
}

//======================== event actions ========================

ACTION marble::newevent(uint64_t serial, name event_name, optional<time_point_sec> custom_event_time, bool shared)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //initialize
    time_point_sec now = time_point_sec(current_time_point());
    time_point_sec new_event_time = (custom_event_time) ? *custom_event_time : now;

    //if shared event
    if (shared) {
        //open shared events table, find shared event
        shared_events_table shared_events(get_self(), grp.group_name.value);
        auto sh_event_itr = shared_events.find(event_name.value);

        //validate
        check(sh_event_itr == shared_events.end(), "shared event already exists");

        //create new shared event
        //ram payer: contract
        shared_events.emplace(get_self(), [&](auto& col) {
            col.event_name = event_name;
            col.event_time = new_event_time;
            col.locked = false;
        });
    } else {
        //open events table, find event
        events_table events(get_self(), serial);
        auto event_itr = events.find(event_name.value);

        //validate
        check(event_itr == events.end(), "event already exists"); 

        //create new event
        //ram payer: self
        events.emplace(get_self(), [&](auto& col) {
            col.event_name = event_name;
            col.event_time = new_event_time;
            col.locked = false;
        });
    }
}

ACTION marble::seteventtime(uint64_t serial, name event_name, time_point_sec new_event_time, bool shared)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //if shared event
    if (shared) {
        //open shared events table, get shared event
        shared_events_table shared_events(get_self(), grp.group_name.value);
        auto& se = shared_events.get(event_name.value, "shared event not found");

        //validate
        check(!se.locked, "shared event is locked");

        //update shared event
        shared_events.modify(se, same_payer, [&](auto& col) {
            col.event_time = new_event_time;
        });
    } else {
        //open events table, get event
        events_table events(get_self(), serial);
        auto& e = events.get(event_name.value, "event not found");

        //validate
        check(!e.locked, "event is locked");

        //update event
        events.modify(e, same_payer, [&](auto& col) {
            col.event_time = new_event_time;
        });
    }
}

ACTION marble::lockevent(uint64_t serial, name event_name, bool shared)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //if shared event
    if (shared) {
        //open shared events table, get shared event
        shared_events_table shared_events(get_self(), grp.group_name.value);
        auto& se = shared_events.get(event_name.value, "shared event not found");

        //validate
        check(!se.locked, "shared event is locked");

        //update shared event
        shared_events.modify(se, same_payer, [&](auto& col) {
            col.locked = true;
        });
    } else {
        //open events table, get event
        events_table events(get_self(), serial);
        auto& e = events.get(event_name.value, "event not found");

        //validate
        check(!e.locked, "event is already locked");

        //update event
        events.modify(e, same_payer, [&](auto& col) {
            col.locked = true;
        });
    }
}

ACTION marble::rmvevent(uint64_t serial, name event_name, bool shared)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //if shared event
    if (shared) {
        //open shared events table, get shared event
        shared_events_table shared_events(get_self(), grp.group_name.value);
        auto& se = shared_events.get(event_name.value, "shared event not found");

        //erase shared event
        shared_events.erase(se);
    } else {
        //open events table, get event
        events_table events(get_self(), serial);
        auto& e = events.get(event_name.value, "event not found");

        //erase event
        events.erase(e);
    }
}

ACTION marble::logevent(name event_name, int64_t event_value, time_point_sec event_time, string memo, bool shared)
{
    //authenticate
    require_auth(get_self()); //TODO: permission_level{get_self(), name("log")}
}

//======================== bond actions ========================

ACTION marble::newbond(uint64_t serial, asset amount, optional<name> release_event)
{
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

    //open wallets table, get manager wallet
    wallets_table wallets(get_self(), grp.manager.value);
    auto& mgr_wall = wallets.get(amount.symbol.code().raw(), "manager wallet not found");

    //validate
    check(mgr_wall.balance >= amount, "insufficient funds");
    check(amount.amount > 0, "must back with a positive amount");

    //subtract from wallet balance
    wallets.modify(mgr_wall, same_payer, [&](auto& col) {
        col.balance -= amount;
    });

    //open bonds table, search for bond
    bonds_table bonds(get_self(), serial);
    auto bond_itr = bonds.find(amount.symbol.code().raw());

    //validate
    check(bond_itr == bonds.end(), "bond already exists");

    //initialize
    name bond_release_event = (release_event) ? *release_event : name(0);

    //if release event not blank
    if (bond_release_event != name(0)) {
        //open events table, get event
        events_table events(get_self(), serial);
        auto& evnt = events.get(bond_release_event.value, "release event not found");

        //validate
        check(evnt.event_time > time_point_sec(current_time_point()), "release event time must be in the future");
    }

    //emplace new bond
    //ram payer: contract
    bonds.emplace(get_self(), [&](auto& col) {
        col.backed_amount = amount;
        col.release_event = bond_release_event;
        col.locked = false;
    });
}

ACTION marble::addtobond(uint64_t serial, asset amount)
{
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

    //open wallets table, get manager wallet
    wallets_table wallets(get_self(), grp.manager.value);
    auto& mgr_wall = wallets.get(amount.symbol.code().raw(), "manager wallet not found");

    //validate
    check(mgr_wall.balance >= amount, "insufficient funds");
    check(amount.amount > 0, "must back with a positive amount");

    //subtract from wallet balance
    wallets.modify(mgr_wall, same_payer, [&](auto& col) {
        col.balance -= amount;
    });

    //open bonds table, search for bond
    bonds_table bonds(get_self(), serial);
    auto& bnd = bonds.get(amount.symbol.code().raw(), "bond not found");

    //validate
    check(!bnd.locked, "bond cannot be modified if locked");

    //update bond
    bonds.modify(bnd, same_payer, [&](auto& col) {
        col.backed_amount += amount;
    });
}

ACTION marble::release(uint64_t serial)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //authenticate
    require_auth(itm.owner);

    //open bonds table, get bond
    bonds_table bonds(get_self(), serial);
    auto& bnd = bonds.get(CORE_SYM.code().raw(), "bond not found");

    //validate
    check(bnd.release_event != name(0), "bond must have a release event to manually release");

    //initialize
    time_point_sec now = time_point_sec(current_time_point());

    //open events table, get event
    events_table events(get_self(), serial);
    auto& evnt = events.get(bnd.release_event.value, "event not found");

    //validate
    check(now >= evnt.event_time, "bond can only be released after release event time");

    //open wallets table, search for wallet
    wallets_table wallets(get_self(), itm.owner.value);
    auto wall_itr = wallets.find(CORE_SYM.code().raw());

    //if wallet found
    if (wall_itr != wallets.end()) {
        //add to existing wallet
        wallets.modify(*wall_itr, same_payer, [&](auto& col) {
            col.balance += bnd.backed_amount;
        });
    } else {
        //create new wallet
        //ram payer: contract
        wallets.emplace(get_self(), [&](auto& col) {
            col.balance = bnd.backed_amount;
        });
    }

    //erase bond
    bonds.erase(bnd);
}

ACTION marble::releaseall(uint64_t serial, name release_to)
{
    //authenticate
    // require_auth(permission_level{get_self(), name("releases")});
    require_auth(get_self());

    //open bonds table, get bond
    bonds_table bonds(get_self(), serial);
    auto& bnd = bonds.get(CORE_SYM.code().raw(), "bond not found");

    //open wallets table, search for wallet
    wallets_table wallets(get_self(), release_to.value);
    auto wall_itr = wallets.find(CORE_SYM.code().raw());

    //if wallet found
    if (wall_itr != wallets.end()) {
        //add to existing wallet
        wallets.modify(*wall_itr, same_payer, [&](auto& col) {
            col.balance += bnd.backed_amount;
        });
    } else {
        //create new wallet
        //ram payer: contract
        wallets.emplace(get_self(), [&](auto& col) {
            col.balance = bnd.backed_amount;
        });
    }

    //erase bond
    bonds.erase(bnd);
}

ACTION marble::lockbond(uint64_t serial)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open bonds table, get bond
    bonds_table bonds(get_self(), serial);
    auto& bnd = bonds.get(CORE_SYM.code().raw(), "bond not found");

    //validate
    check(!bnd.locked, "bond is already locked");

    //update bond
    bonds.modify(bnd, same_payer, [&](auto& col) {
        col.locked = true;
    });
}

//======================== frame actions ========================

ACTION marble::newframe(name frame_name, name group, map<name, string> default_tags, map<name, int64_t> default_attributes)
{
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

ACTION marble::applyframe(name frame_name, uint64_t serial, bool overwrite)
{
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

ACTION marble::quickbuild(name frame_name, name to, map<name, string> override_tags, map<name, int64_t> override_attributes)
{
    //open frames table, get frame
    frames_table frames(get_self(), get_self().value);
    auto& frm = frames.get(frame_name.value, "frame not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(frm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open config table, get config
    config_table configs(get_self(), get_self().value);
    auto conf = configs.get();

    //initialize
    uint64_t item_serial = conf.last_serial + 1;

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

ACTION marble::cleanframe(name frame_name, uint64_t serial)
{
    //open frames table, get frame
    frames_table frames(get_self(), get_self().value);
    auto& frm = frames.get(frame_name.value, "frame not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(frm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //clean default tags
    for (auto itr = frm.default_tags.begin(); itr != frm.default_tags.end(); itr++) {
        //open tags table, find tag
        tags_table tags(get_self(), serial);
        auto tag_itr = tags.find(itr->first.value);

        //if tag found
        if (tag_itr != tags.end()) {
            //delete tag
            tags.erase(*tag_itr);
        }
    }

    //clean default attributes
    for (auto itr = frm.default_attributes.begin(); itr != frm.default_attributes.end(); itr++) {
        //open attributes table, find attribute
        attributes_table attributes(get_self(), serial);
        auto attr_itr = attributes.find(itr->first.value);

        //if attribute found
        if (attr_itr == attributes.end()) {
            //delete attribute
            attributes.erase(*attr_itr);
        }
    }

    //clean default events
    // for (auto itr = frm.default_events.begin(); itr != frm.default_events.end(); itr++) {
    //     //open events table, find event
    //     events_table events(get_self(), serial);
    //     auto event_itr = events.find(itr->first.value);

    //     //if event found
    //     if (event_itr == events.end()) {
    //         //delete event
    //         events.erase(*event_itr);
    //     }
    // }
}

ACTION marble::rmvframe(name frame_name, string memo)
{
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

//======================== wallet actions ========================

ACTION marble::withdraw(name wallet_owner, asset amount)
{
    //validate
    check(amount.symbol == CORE_SYM, "can only withdraw core token");
    
    //authenticate
    require_auth(wallet_owner);

    //open wallets table, get deposit
    wallets_table wallets(get_self(), wallet_owner.value);
    auto& wall = wallets.get(amount.symbol.code().raw(), "wallet not found");

    //validate
    check(amount.amount > 0, "must withdraw a positive amount");
    check(wall.balance >= amount, "insufficient funds");

    //if withdrawing all tokens
    if (wall.balance == amount) {
        //erase wallet
        wallets.erase(wall);
    } else {
        //update wallet balance
        wallets.modify(wall, same_payer, [&](auto& col) {
            col.balance -= amount;
        });
    }

    //send inline eosio.token::transfer to withdrawing account
    //auth: self
    action(permission_level{get_self(), name("active")}, name("eosio.token"), name("transfer"), make_tuple(
        get_self(), //from
        wallet_owner, //to
        amount, //quantity
        std::string("Marble Wallet Withdrawal") //memo
    )).send();
}

//======================== notification handlers ========================

void marble::catch_transfer(name from, name to, asset quantity, string memo)
{
    //get initial receiver contract
    name rec = get_first_receiver();

    //if received notification from eosio.token, not from self, and symbol is CORE SYM
    if (rec == name("eosio.token") && from != get_self() && quantity.symbol == CORE_SYM) {
        //if memo is "deposit"
        if (memo == std::string("deposit")) { 
            //open wallets table, search for wallet
            wallets_table wallets(get_self(), from.value);
            auto wall_itr = wallets.find(CORE_SYM.code().raw());

            //if wallet found
            if (wall_itr != wallets.end()) {
                //add to existing wallet
                wallets.modify(*wall_itr, same_payer, [&](auto& col) {
                    col.balance += quantity;
                });
            } else {
                //create new wallet
                //ram payer: contract
                wallets.emplace(get_self(), [&](auto& col) {
                    col.balance = quantity;
                });
            }
        }
    }
}
