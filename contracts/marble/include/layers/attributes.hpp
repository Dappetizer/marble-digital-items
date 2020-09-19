//layer name: attributes
//required: groups, items

//======================== attribute actions ========================

//assign a new attribute to an item
//auth: manager
ACTION newattribute(uint64_t serial, name attribute_name, int64_t initial_points, bool shared);

//sets an attributes points
//auth: manager
ACTION setpoints(uint64_t serial, name attribute_name, int64_t new_points, bool shared);

//increases attribute points by amount
//auth: manager
ACTION increasepts(uint64_t serial, name attribute_name, uint64_t points_to_add, bool shared);

//decreases attribute points by amount
//auth: manager
ACTION decreasepts(uint64_t serial, name attribute_name, uint64_t points_to_subtract, bool shared);

//locks an attribute to prevent mutations
//auth: manager
ACTION lockattr(uint64_t serial, name attribute_name, bool shared);

//removes an attribute from an item
//auth: manager
ACTION rmvattribute(uint64_t serial, name group_name, name attribute_name, bool shared);

//======================== attribute tables ========================

//attributes table
//scope: serial
//ram payer: manager
TABLE attribute {
    name attribute_name;
    int64_t points;
    bool locked;

    uint64_t primary_key() const { return attribute_name.value; }

    EOSLIB_SERIALIZE(attribute, (attribute_name)(points)(locked))
};
typedef multi_index<name("attributes"), attribute> attributes_table;

//shared attributes table
//scope: group
//ram payer: manager
TABLE shared_attribute {
    name attribute_name;
    int64_t points;
    bool locked;

    uint64_t primary_key() const { return attribute_name.value; }

    EOSLIB_SERIALIZE(shared_attribute, (attribute_name)(points)(locked))
};
typedef multi_index<name("sharedattrs"), shared_attribute> shared_attributes_table;
