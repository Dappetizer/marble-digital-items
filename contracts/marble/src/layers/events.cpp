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

ACTION marble::rmvevent(uint64_t serial, name group_name, name event_name, bool shared)
{
    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(group_name.value, "group not found");

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
