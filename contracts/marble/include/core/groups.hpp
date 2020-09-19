//layer name: groups
//required: config

//======================== group actions ========================

//create a new item group
//auth: admin
ACTION newgroup(string title, string description, name group_name, name manager, uint64_t supply_cap);

//edit group title and description
//auth: manager
ACTION editgroup(name group_name, string new_title, string new_description);

//set a new group manager
//auth: manager
ACTION setmanager(name group_name, name new_manager, string memo);

//======================== group tables ========================

TABLE group {
    string title;
    string description;
    name group_name;
    name manager;
    uint64_t supply;
    uint64_t issued_supply;
    uint64_t supply_cap;

    uint64_t primary_key() const { return group_name.value; }

    EOSLIB_SERIALIZE(group, (title)(description)(group_name)(manager)
        (supply)(issued_supply)(supply_cap))
};
typedef multi_index<name("groups"), group> groups_table;