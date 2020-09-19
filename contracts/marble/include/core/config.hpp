//layer name: config
//required: none

//======================== config actions ========================

//initialize the contract
//auth: self
ACTION init(string contract_name, string contract_version, name initial_admin);

//set new contract version
//auth: admin
ACTION setversion(string new_version);

//set new admin
//auth: admin
ACTION setadmin(name new_admin);

//assume RAM costs for a table row
//auth: payer
//ACTION payram(name payer, name table_name, name key);

//======================== config tables ========================

//config table
//scope: self
//ram payer: contract
TABLE config {
    string contract_name;
    string contract_version;
    name admin;
    uint64_t last_serial;
    //uint64_t last_locker_id;
    //symbol core_sym;
    //vector<name> installed; //name of layers installed on marble factory

    EOSLIB_SERIALIZE(config, (contract_name)(contract_version)(admin)(last_serial))
};
typedef singleton<name("config"), config> config_table;