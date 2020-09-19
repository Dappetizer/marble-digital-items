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
