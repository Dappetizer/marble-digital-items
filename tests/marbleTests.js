//eoslime
const eoslime = require("eoslime").init("local");
const assert = require('assert');

//contracts
const MARBLE_WASM = "./build/marble/marble.wasm";
const MARBLE_ABI = "./build/marble/marble.abi";

describe("Marble Digital Items Tests", function () {
    //increase mocha testing timeframe
    this.timeout(15000);

    //base tester
    before(async () => {
        //create marble accounts
        marbleAccount = await eoslime.Account.createFromName("mbl")
        sandboxAccount = await eoslime.Account.createFromName("sandbox.mbl");
        adminAccount = await eoslime.Account.createFromName("admin.mbl");

        //create test accounts
        testAccount1 = await eoslime.Account.createFromName("testaccount1");
        testAccount2 = await eoslime.Account.createFromName("testaccount2");
        testAccount3 = await eoslime.Account.createFromName("testaccount3");

        //deploy marble contract
        marbleContract = await eoslime.Contract.deployOnAccount(
            MARBLE_WASM,
            MARBLE_ABI,
            marbleAccount
        );

        //add eosio.code permission to mbl@active
        await marbleAccount.addPermission('eosio.code');

        //initialize
        const contractName = "Marble Digital Items";
        const contractVersion = "v1.3.0";

        //call init() on marble contract
        const res = await marbleContract.actions.init([contractName, contractVersion, marbleAccount.name], {from: marbleAccount});
        assert(res.processed.receipt.status == 'executed', "init() action was not executed");

        //assert config table values
        const confTable = await marbleContract.provider.select('config').from('mbl').find();
        assert(confTable[0].contract_name == contractName, "Incorrect Contract Name");
        assert(confTable[0].contract_version == contractVersion, "Incorrect Contract Version");
        assert(confTable[0].admin == marbleAccount.name, "Incorrect Admin");
        // assert(confTable[0].last_serial == 0, "Incorrect Last Serial");
    });

    it("Change Version", async () => {
        //initialize
        const newVersion = "2.0.0";

        //call setversion() on marble contract
        const res = await marbleContract.actions.setversion([newVersion], {from: marbleAccount});
        assert(res.processed.receipt.status == 'executed', "setversion() action was not executed");

        //assert config table values
        const confTable = await marbleContract.provider.select('config').from('mbl').find();
        assert(confTable[0].contract_version == newVersion, "Incorrect Contract Version");
    });

    it("Change Admin", async () => {
        //call setadmin() on marble contract
        const res = await marbleContract.actions.setadmin([adminAccount.name], {from: marbleAccount});
        assert(res.processed.receipt.status == 'executed', "setadmin() action was not executed");

        //assert group table values
        const confTable = await marbleContract.provider.select('config').from('mbl').find();
        assert(confTable[0].admin == adminAccount.name, "Incorrect Admin Account");
    });

    it("Create New Group", async () => {
        //initialize
        const groupTitle = "Marble Heroes";
        const groupDesc = "A collection of heroes using Marble";
        const groupName = "heroes";
        const groupManager = testAccount1.name;
        const groupSupplyCap = 100;

        //call newgroup() on marble contract
        const res = await marbleContract.actions.newgroup([groupTitle, groupDesc, groupName, groupManager, groupSupplyCap], {from: adminAccount});
        assert(res.processed.receipt.status == 'executed', "newgroup() action was not executed");

        //assert group table values
        const groupsTable = await marbleContract.provider.select('groups').from('mbl').find('heroes');
        assert(groupsTable[0].title == groupTitle, "Incorrect Group Title");
        assert(groupsTable[0].description == groupDesc, "Incorrect Group Descrption");
        assert(groupsTable[0].group_name == groupName, "Incorrect Group Name");
        assert(groupsTable[0].manager == groupManager, "Incorrect Group Manager");
        assert(groupsTable[0].supply == 0, "Incorrect Supply");
        assert(groupsTable[0].issued_supply == 0, "Incorrect Issued Supply");
        assert(groupsTable[0].supply_cap == groupSupplyCap, "Incorrect Supply Cap");

        //assert behavior table values
        // const behaviorsTable = await marbleContract.provider.select('behaviors').from('mbl').scope('heroes').find();
    });
    
});