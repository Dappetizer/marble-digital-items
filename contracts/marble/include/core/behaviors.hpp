//======================== behavior actions ========================

//add a behavior to a group
//auth: manager
ACTION addbehavior(name group_name, name behavior_name, bool initial_state);

//toggle a behavior on/off
//auth: manager
ACTION togglebhvr(name group_name, name behavior_name);

//lock a behavior to prevent mutations
//auth: manager
ACTION lockbhvr(name group_name, name behavior_name);

//remove a behavior from a group
//auth: manager
ACTION rmvbehavior(name group_name, name behavior_name);

//======================== behavior tables ========================

//behaviors table
//scope: group
//ram payer: manager
TABLE behavior {
    name behavior_name;
    bool state;
    bool locked;

    uint64_t primary_key() const { return behavior_name.value; }

    EOSLIB_SERIALIZE(behavior, (behavior_name)(state)(locked))
};
typedef multi_index<name("behaviors"), behavior> behaviors_table;
