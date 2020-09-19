//layer name: bonds
//required: groups, items, events, wallets

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
