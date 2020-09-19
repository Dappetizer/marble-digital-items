//layer name: Wallets
//required: core

//======================== wallet actions ========================

//withdraw tokens from a wallet
//pre: balance >= withdrawal amount
//post: if remaining balance = 0 then delete wallet
//auth: wallet owner
ACTION withdraw(name wallet_owner, asset amount);

//withdraw items from a locker
//auth: locker owner
// ACTION withdrawitem(name locker_owner, name factory_name, uint64_t tracker_id);

//======================== notification handlers ========================

//catch a transfer() from eosio.token
[[eosio::on_notify("eosio.token::transfer")]]
void catch_transfer(name from, name to, asset quantity, string memo);

//catch a transferitem() from an approved factory contract
// [[eosio::on_notify("eosio.token::transfer")]]
// void catch_transferitem(name from, name to, asset quantity, string memo);

//======================== wallet tables ========================

//wallets table
//scope: owner
//ram payer: owner
TABLE wallet {
    asset balance; //wallet balance

    uint64_t primary_key() const { return balance.symbol.code().raw(); }

    EOSLIB_SERIALIZE(wallet, (balance))
};
typedef multi_index<name("wallets"), wallet> wallets_table;

//lockers tables
//scope: self
//ram payer: contract
// TABLE locker {
//     uint64_t locker_id;
//     name owner;
//     uint64_t tracker_id;
// };
// typedef multi_index<name("lockers"), locker> lockers_table;

//currencies table
//scope: self
//ram payer: contract
// TABLE currency {
//     asset total_deposits; //total deposited assets across all wallets
//     uint32_t total_wallets; //total unique wallets
//     name home_contract; //account where currency contract is deployed
//     //string contract_standard;
//     bool approved; //allows currency deposits

//     uint64_t primary_key() const { return total_deposits.symbol.code().raw(); }

//     EOSLIB_SERIALIZE(currency, (total_deposits)(total_wallets)(home_contract)(approved))
// };
// typedef multi_index<name("currencies"), currency> currencies_table;

//factories table
//scope: self
//ram payer: contract
// TABLE factory {
//     name home_contract; //account where marble contract is deployed
//     uint64_t total_deposits; //total items deposited into lockers
//     uint64_t total_lockers; //total unique lockers
//     bool approved; //allows item deposits
// };
// typedef multi_index<name("factories"), factory> factories_table;
