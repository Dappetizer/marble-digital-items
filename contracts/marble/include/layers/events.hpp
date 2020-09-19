//layer name: events
//required: groups, items

//======================== event actions ========================

//create a new event (if no custom event time given use current time point)
//auth: manager
ACTION newevent(uint64_t serial, name event_name, optional<time_point_sec> custom_event_time, bool shared);

//set a custom time on an event
//auth: manager
ACTION seteventtime(uint64_t serial, name event_name, time_point_sec new_event_time, bool shared);

//lock an event time to prevent mutations
//auth: manager
ACTION lockevent(uint64_t serial, name event_name, bool shared);

//remove an event
//auth: manager
ACTION rmvevent(uint64_t serial, name group_name, name event_name, bool shared);

//log an event (will not save to events table)
//auth: self
ACTION logevent(name event_name, int64_t event_value, time_point_sec event_time, string memo, bool shared);

//======================== event tables ========================

//events table
//scope: serial
//ram payer: manager
TABLE event {
    name event_name;
    time_point_sec event_time;
    bool locked;

    uint64_t primary_key() const { return event_name.value; }

    EOSLIB_SERIALIZE(event, (event_name)(event_time)(locked))
};
typedef multi_index<name("events"), event> events_table;

//shared events table
//scope: group
//ram payer: manager
TABLE shared_event {
    name event_name;
    time_point_sec event_time;
    bool locked;

    uint64_t primary_key() const { return event_name.value; }

    EOSLIB_SERIALIZE(shared_event, (event_name)(event_time)(locked))
};
typedef multi_index<name("sharedevents"), shared_event> shared_events_table;
