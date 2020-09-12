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
