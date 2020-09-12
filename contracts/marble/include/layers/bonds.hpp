//======================== bond actions ========================

//back an item with a fungible token (draws from manager wallet balance)
//auth: manager
ACTION newbond(uint64_t serial, asset amount, optional<name> release_event);

//add more tokens to an existing bond
//pre: bond exists, bond not locked
//auth: manager
ACTION addtobond(uint64_t serial, asset amount);

//release preconfigured amount from item bond
//pre: item exists, release conditions met
//auth: item owner
ACTION release(uint64_t serial);

//release all bond amounts from an item
//pre: item consumed or destroyed, release_to == item.owner
//auth: contract (inline)
ACTION releaseall(uint64_t serial, name release_to);

//locks a bond to prevent settings changes
//pre: bond not locked
//auth: manager
ACTION lockbond(uint64_t serial);

//======================== wallet actions ========================

//withdraw tokens from a wallet
//auth: wallet owner
ACTION withdraw(name wallet_owner, asset amount);

//======================== notification handlers ========================

//catch a transfer from eosio.token
[[eosio::on_notify("eosio.token::transfer")]]
void catch_transfer(name from, name to, asset quantity, string memo);

//======================== bond tables ========================

//bonds table
//scope: serial
//ram payer: manager
TABLE bond {
    asset backed_amount; //token amount stored by bond
    name release_event; //event name storing release time (blank for no release time)
    bool locked; //if true bond settings cannot be changed

    // asset per_release; //amount released from bond per release event
    // uint16_t steps; //number of release steps before maturity
    // asset per_step; //amount released from bond per step

    uint64_t primary_key() const { return backed_amount.symbol.code().raw(); }

    EOSLIB_SERIALIZE(bond, (backed_amount)(release_event)(locked))
};
typedef multi_index<name("bonds"), bond> bonds_table;

//wallets table
//scope: owner
//ram payer: owner
TABLE wallet {
    asset balance; //wallet balance

    uint64_t primary_key() const { return balance.symbol.code().raw(); }

    EOSLIB_SERIALIZE(wallet, (balance))
};
typedef multi_index<name("wallets"), wallet> wallets_table;

//currencies table
//scope: self
//ram payer: contract
TABLE currency {
    asset deposits; //total deposited assets across wallets
    uint32_t wallets; //total unique wallets with balance
    name account; //account where currency contract is deployed
    //asset bonded; //total assets tied up in bonds

    uint64_t primary_key() const { return deposits.symbol.code().raw(); }

    EOSLIB_SERIALIZE(currency, (deposits)(wallets)(account))
};
typedef multi_index<name("currencies"), currency> currencies_table;