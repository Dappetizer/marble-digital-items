//layer name: tags
//required: groups, items

//======================== tag actions ========================

//assign a new tag to an item
//auth: manager
ACTION newtag(uint64_t serial, name tag_name, string content, optional<string> checksum, optional<string> algorithm, bool shared);

//update tag content, checksum, and/or algorithm
//auth: manager
ACTION updatetag(uint64_t serial, name tag_name, string new_content, optional<string> new_checksum, optional<string> new_algorithm, bool shared);

//lock a tag to prevent mutations
//auth: manager
ACTION locktag(uint64_t serial, name tag_name, bool shared);

//remove tag from item
//auth: manager
ACTION rmvtag(uint64_t serial, name group_name, name tag_name, string memo, bool shared);

//======================== tag tables ========================

//tags table
//scope: serial
//ram payer: manager
TABLE tag {
    name tag_name;
    string content;
    string checksum;
    string algorithm;
    bool locked;

    uint64_t primary_key() const { return tag_name.value; }

    EOSLIB_SERIALIZE(tag, (tag_name)(content)(checksum)(algorithm)(locked))
};
typedef multi_index<"tags"_n, tag> tags_table;

//shared tags table
//scope: group
//ram payer: manager
TABLE shared_tag {
    name tag_name;
    string content;
    string checksum;
    string algorithm;
    bool locked;

    uint64_t primary_key() const { return tag_name.value; }

    EOSLIB_SERIALIZE(shared_tag, (tag_name)(content)(checksum)(algorithm)(locked))
};
typedef multi_index<name("sharedtags"), shared_tag> shared_tags_table;
