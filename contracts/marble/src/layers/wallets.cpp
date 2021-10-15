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
    if (rec == name("eosio.token") && from != get_self() && to == get_self() && quantity.symbol == CORE_SYM) {
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