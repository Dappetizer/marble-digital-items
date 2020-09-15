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

    //TODO: prevent removal if locked?

    //erase behavior
    behaviors.erase(bhvr);
}
