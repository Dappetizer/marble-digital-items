//layer name: frames
//required: groups, items, tags, attributes

//======================== frame actions ========================

//set up a new frame
//auth: manager
ACTION newframe(name frame_name, name group, map<name, string> default_tags, map<name, int64_t> default_attributes);

//applies a frame to an item
//auth: manager
ACTION applyframe(name frame_name, uint64_t serial, bool overwrite);

//mints a new item and applies a frame immediately with tag and attribute default overrides
//auth: manager
ACTION quickbuild(name frame_name, name to, map<name, string> override_tags, map<name, int64_t> override_attributes);

//cleans a frame from an item
//auth: manager
ACTION cleanframe(name frame_name, uint64_t serial);

//remove a frame
//auth: manager
ACTION rmvframe(name frame_name, string memo);

//======================== frame tables ========================

//frames table
//scope: self
//ram payer: manager
TABLE frame {
    name frame_name;
    name group;
    map<name, string> default_tags; //tag_name => default content
    map<name, int64_t> default_attributes; //attribute_name => default value
    // map<name, time_point_sec> default_events; //event_name => default value
    // bool shared;

    uint64_t primary_key() const { return frame_name.value; }
    uint64_t by_group() const { return group.value; }
    EOSLIB_SERIALIZE(frame, (frame_name)(group)(default_tags)(default_attributes))
};
typedef multi_index<"frames"_n, frame,
    indexed_by<"bygroup"_n, const_mem_fun<frame, uint64_t, &frame::by_group>>
> frames_table;
