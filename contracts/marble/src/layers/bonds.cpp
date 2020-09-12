//======================== bond actions ========================

ACTION marble::newbond(uint64_t serial, asset amount, optional<name> release_event)
{
    //validate
    check(amount.symbol == CORE_SYM, "asset must be core symbol");

    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open wallets table, get manager wallet
    wallets_table wallets(get_self(), grp.manager.value);
    auto& mgr_wall = wallets.get(amount.symbol.code().raw(), "manager wallet not found");

    //validate
    check(mgr_wall.balance >= amount, "insufficient funds");
    check(amount.amount > 0, "must back with a positive amount");

    //subtract from wallet balance
    wallets.modify(mgr_wall, same_payer, [&](auto& col) {
        col.balance -= amount;
    });

    //open bonds table, search for bond
    bonds_table bonds(get_self(), serial);
    auto bond_itr = bonds.find(amount.symbol.code().raw());

    //validate
    check(bond_itr == bonds.end(), "bond already exists");

    //initialize
    name bond_release_event = (release_event) ? *release_event : name(0);

    //if release event not blank
    if (bond_release_event != name(0)) {
        //open events table, get event
        events_table events(get_self(), serial);
        auto& evnt = events.get(bond_release_event.value, "release event not found");

        //validate
        check(evnt.event_time > time_point_sec(current_time_point()), "release event time must be in the future");
    }

    //emplace new bond
    //ram payer: contract
    bonds.emplace(get_self(), [&](auto& col) {
        col.backed_amount = amount;
        col.release_event = bond_release_event;
        col.locked = false;
    });
}

ACTION marble::addtobond(uint64_t serial, asset amount)
{
    //validate
    check(amount.symbol == CORE_SYM, "asset must be core symbol");

    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open wallets table, get manager wallet
    wallets_table wallets(get_self(), grp.manager.value);
    auto& mgr_wall = wallets.get(amount.symbol.code().raw(), "manager wallet not found");

    //validate
    check(mgr_wall.balance >= amount, "insufficient funds");
    check(amount.amount > 0, "must back with a positive amount");

    //subtract from wallet balance
    wallets.modify(mgr_wall, same_payer, [&](auto& col) {
        col.balance -= amount;
    });

    //open bonds table, search for bond
    bonds_table bonds(get_self(), serial);
    auto& bnd = bonds.get(amount.symbol.code().raw(), "bond not found");

    //validate
    check(!bnd.locked, "bond cannot be modified if locked");

    //update bond
    bonds.modify(bnd, same_payer, [&](auto& col) {
        col.backed_amount += amount;
    });
}

ACTION marble::release(uint64_t serial)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //authenticate
    require_auth(itm.owner);

    //open bonds table, get bond
    bonds_table bonds(get_self(), serial);
    auto& bnd = bonds.get(CORE_SYM.code().raw(), "bond not found");

    //validate
    check(bnd.release_event != name(0), "bond must have a release event to manually release");

    //initialize
    time_point_sec now = time_point_sec(current_time_point());

    //open events table, get event
    events_table events(get_self(), serial);
    auto& evnt = events.get(bnd.release_event.value, "event not found");

    //validate
    check(now >= evnt.event_time, "bond can only be released after release event time");

    //open wallets table, search for wallet
    wallets_table wallets(get_self(), itm.owner.value);
    auto wall_itr = wallets.find(CORE_SYM.code().raw());

    //if wallet found
    if (wall_itr != wallets.end()) {
        //add to existing wallet
        wallets.modify(*wall_itr, same_payer, [&](auto& col) {
            col.balance += bnd.backed_amount;
        });
    } else {
        //create new wallet
        //ram payer: contract
        wallets.emplace(get_self(), [&](auto& col) {
            col.balance = bnd.backed_amount;
        });
    }

    //erase bond
    bonds.erase(bnd);
}

ACTION marble::releaseall(uint64_t serial, name release_to)
{
    //authenticate
    // require_auth(permission_level{get_self(), name("releases")});
    require_auth(get_self());

    //open bonds table, get bond
    bonds_table bonds(get_self(), serial);
    auto& bnd = bonds.get(CORE_SYM.code().raw(), "bond not found");

    //open wallets table, search for wallet
    wallets_table wallets(get_self(), release_to.value);
    auto wall_itr = wallets.find(CORE_SYM.code().raw());

    //if wallet found
    if (wall_itr != wallets.end()) {
        //add to existing wallet
        wallets.modify(*wall_itr, same_payer, [&](auto& col) {
            col.balance += bnd.backed_amount;
        });
    } else {
        //create new wallet
        //ram payer: contract
        wallets.emplace(get_self(), [&](auto& col) {
            col.balance = bnd.backed_amount;
        });
    }

    //erase bond
    bonds.erase(bnd);
}

ACTION marble::lockbond(uint64_t serial)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open bonds table, get bond
    bonds_table bonds(get_self(), serial);
    auto& bnd = bonds.get(CORE_SYM.code().raw(), "bond not found");

    //validate
    check(!bnd.locked, "bond is already locked");

    //update bond
    bonds.modify(bnd, same_payer, [&](auto& col) {
        col.locked = true;
    });
}

//======================== wallet actions ========================

ACTION marble::withdraw(name wallet_owner, asset amount)
{
    //validate
    check(amount.symbol == CORE_SYM, "can only withdraw core token");
    
    //authenticate
    require_auth(wallet_owner);

    //open wallets table, get deposit
    wallets_table wallets(get_self(), wallet_owner.value);
    auto& wall = wallets.get(amount.symbol.code().raw(), "wallet not found");

    //validate
    check(amount.amount > 0, "must withdraw a positive amount");
    check(wall.balance >= amount, "insufficient funds");

    //if withdrawing all tokens
    if (wall.balance == amount) {
        //erase wallet
        wallets.erase(wall);
    } else {
        //update wallet balance
        wallets.modify(wall, same_payer, [&](auto& col) {
            col.balance -= amount;
        });
    }

    //send inline eosio.token::transfer to withdrawing account
    //auth: self
    action(permission_level{get_self(), name("active")}, name("eosio.token"), name("transfer"), make_tuple(
        get_self(), //from
        wallet_owner, //to
        amount, //quantity
        std::string("Marble Wallet Withdrawal") //memo
    )).send();
}

//======================== notification handlers ========================

void marble::catch_transfer(name from, name to, asset quantity, string memo)
{
    //get initial receiver contract
    name rec = get_first_receiver();

    //if received notification from eosio.token, not from self, and symbol is CORE SYM
    if (rec == name("eosio.token") && from != get_self() && quantity.symbol == CORE_SYM) {
        //if memo is "deposit"
        if (memo == std::string("deposit")) { 
            //open wallets table, search for wallet
            wallets_table wallets(get_self(), from.value);
            auto wall_itr = wallets.find(CORE_SYM.code().raw());

            //if wallet found
            if (wall_itr != wallets.end()) {
                //add to existing wallet
                wallets.modify(*wall_itr, same_payer, [&](auto& col) {
                    col.balance += quantity;
                });
            } else {
                //create new wallet
                //ram payer: contract
                wallets.emplace(get_self(), [&](auto& col) {
                    col.balance = quantity;
                });
            }
        }
    }
}
