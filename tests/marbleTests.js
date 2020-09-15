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
        assert(groupsTable[0].description == groupDesc, "Incorrect Group Description");
        assert(groupsTable[0].group_name == groupName, "Incorrect Group Name");
        assert(groupsTable[0].manager == groupManager, "Incorrect Group Manager");
        assert(groupsTable[0].supply == 0, "Incorrect Supply");
        assert(groupsTable[0].issued_supply == 0, "Incorrect Issued Supply");
        assert(groupsTable[0].supply_cap == groupSupplyCap, "Incorrect Supply Cap");

        //assert behavior table values
        const behaviorsTable = await marbleContract.provider.select('behaviors').from('mbl').scope('heroes').limit(10).find();
        assert(behaviorsTable[0].behavior_name == "activate", "Incorrect Behavior Name");
        assert(behaviorsTable[0].state == false, "Incorrect Behavior State");
        assert(behaviorsTable[0].locked == false, "Incorrect Behavior Lock State");

        assert(behaviorsTable[1].behavior_name == "consume", "Incorrect Behavior Name");
        assert(behaviorsTable[1].state == false, "Incorrect Behavior State");
        assert(behaviorsTable[1].locked == false, "Incorrect Behavior Lock State");

        assert(behaviorsTable[2].behavior_name == "destroy", "Incorrect Behavior Name");
        assert(behaviorsTable[2].state == true, "Incorrect Behavior State");
        assert(behaviorsTable[2].locked == false, "Incorrect Behavior Lock State");

        assert(behaviorsTable[3].behavior_name == "mint", "Incorrect Behavior Name");
        assert(behaviorsTable[3].state == true, "Incorrect Behavior State");
        assert(behaviorsTable[3].locked == false, "Incorrect Behavior Lock State");

        assert(behaviorsTable[4].behavior_name == "reclaim", "Incorrect Behavior Name");
        assert(behaviorsTable[4].state == false, "Incorrect Behavior State");
        assert(behaviorsTable[4].locked == false, "Incorrect Behavior Lock State");

        assert(behaviorsTable[5].behavior_name == "transfer", "Incorrect Behavior Name");
        assert(behaviorsTable[5].state == true, "Incorrect Behavior State");
        assert(behaviorsTable[5].locked == false, "Incorrect Behavior Lock State");
    });

    it("Edit Group Details", async () => {
        //initialize
        const groupName = "heroes";
        const newGroupTitle = "Cool Marble Heroes";
        const newGroupDesc = "A cool collection of heroes using Marble";

        //call editgroup() on marble contract
        const res = await marbleContract.actions.editgroup([groupName, newGroupTitle, newGroupDesc], {from: testAccount1});
        assert(res.processed.receipt.status == 'executed', "editgroup() action was not executed");

        //assert group table values
        const groupsTable = await marbleContract.provider.select('groups').from('mbl').find('heroes');
        assert(groupsTable[0].title == newGroupTitle, "Incorrect Group Title");
        assert(groupsTable[0].description == newGroupDesc, "Incorrect Group Description");
    });

    it("Set Group Manager", async () => {
        //initialize
        const groupName = "heroes";
        const newManager = testAccount2.name;
        const memo = "";

        //call setmanager() on marble contract
        const res = await marbleContract.actions.setmanager([groupName, newManager, memo], {from: testAccount1});
        assert(res.processed.receipt.status == 'executed', "setmanager() action was not executed");

        //assert group table values
        const groupsTable = await marbleContract.provider.select('groups').from('mbl').find('heroes');
        assert(groupsTable[0].manager == newManager, "Incorrect Group Manager");
    });

    it("Add Behavior", async () => {
        //initialize
        const groupName = "heroes";
        const newBehavior = "testbhvr";
        const initialState = false;
        
        //call addbehavior() on marble contract
        const res = await marbleContract.actions.addbehavior([groupName, newBehavior, 0], {from: testAccount2});
        assert(res.processed.receipt.status == 'executed', "addbehavior() action was not executed");

        //assert behavior table values
        const behaviorsTable = await marbleContract.provider.select('behaviors').from('mbl').scope('heroes').equal(newBehavior).find();
        assert(behaviorsTable[0].behavior_name == newBehavior, "Incorrect Behavior Name");
        assert(behaviorsTable[0].state == initialState, "Incorrect Behavior State");
        assert(behaviorsTable[0].locked == false, "Incorrect Behavior Lock State");
    });

    it("Toggle Behavior State", async () => {
        //initialize
        const groupName = "heroes";
        const behaviorName = "testbhvr";
        
        //call togglebhvr() on marble contract
        const res = await marbleContract.actions.togglebhvr([groupName, behaviorName], {from: testAccount2});
        assert(res.processed.receipt.status == 'executed', "togglebhvr() action was not executed");

        //assert behavior table values
        const behaviorsTable = await marbleContract.provider.select('behaviors').from('mbl').scope('heroes').equal(behaviorName).find();
        assert(behaviorsTable[0].behavior_name == behaviorName, "Incorrect Behavior Name");
        assert(behaviorsTable[0].state == true, "Incorrect Behavior State");
    });

    it("Lock Behavior State", async () => {
        //initialize
        const groupName = "heroes";
        const behaviorName = "testbhvr";
        
        //call lockbhvr() on marble contract
        const res = await marbleContract.actions.lockbhvr([groupName, behaviorName], {from: testAccount2});
        assert(res.processed.receipt.status == 'executed', "lockbhvr() action was not executed");

        //assert behavior table values
        const behaviorsTable = await marbleContract.provider.select('behaviors').from('mbl').scope('heroes').equal(behaviorName).find();
        assert(behaviorsTable[0].behavior_name == behaviorName, "Incorrect Behavior Name");
        assert(behaviorsTable[0].locked == true, "Incorrect Behavior Lock State");
    });

    it("Remove Behavior", async () => {
        //initialize
        const groupName = "heroes";
        const behaviorName = "testbhvr";
        
        //call rmvbehavior() on marble contract
        const res = await marbleContract.actions.rmvbehavior([groupName, behaviorName], {from: testAccount2});
        assert(res.processed.receipt.status == 'executed', "rmvbehavior() action was not executed");

        //assert behavior table values
        const behaviorsTable = await marbleContract.provider.select('behaviors').from('mbl').scope('heroes').equal(behaviorName).find();
        assert(behaviorsTable.length == 0, "Behavior Not Removed");
    });

    it("Mint New Item", async () => {
        //initialize
        const toAccount = testAccount1.name;
        const groupName = "heroes";

        //call mintitem() on marble contract
        const res = await marbleContract.actions.mintitem([toAccount, groupName], {from: testAccount2});
        // console.log(res.processed.action_traces[0].inline_traces);
        assert(res.processed.receipt.status == 'executed', "mintitem() action was not executed");

        //assert items table values
        const itemsTable = await marbleContract.provider.select('items').from('mbl').equal(1).find();
        assert(itemsTable[0].serial == 1, "Incorrect Item Serial");
        assert(itemsTable[0].group == groupName, "Incorrect Item Group");
        assert(itemsTable[0].owner == toAccount, "Incorrect Item Owner");

        //assert groups table values
        const groupsTable = await marbleContract.provider.select('groups').from('mbl').find('heroes');
        assert(groupsTable[0].supply == 1, "Incorrect Supply");
        assert(groupsTable[0].issued_supply == 1, "Incorrect Issued Supply");
    });

    it("Transfer Single Item", async () => {
        //initialize
        const fromAccount = testAccount1.name;
        const toAccount = testAccount3.name;
        const serials = [1];
        const memo = "";
        const groupName = "heroes";

        //call transferitem() on marble contract
        const res = await marbleContract.actions.transferitem([fromAccount, toAccount, serials, memo], {from: testAccount1});
        assert(res.processed.receipt.status == 'executed', "transferitem() action was not executed");

        //assert items table values
        const itemsTable = await marbleContract.provider.select('items').from('mbl').equal(1).find();
        assert(itemsTable[0].serial == 1, "Incorrect Item Serial");
        assert(itemsTable[0].group == groupName, "Incorrect Item Group");
        assert(itemsTable[0].owner == toAccount, "Incorrect Item Owner");
    });

    it("Transfer Multiple Items", async () => {
        //initialize
        const fromAccount = testAccount3.name;
        const toAccount = testAccount1.name;
        const serials = [1, 2];
        const memo = "";
        const groupName = "heroes";

        //call mintitem() on marble contract
        await marbleContract.actions.mintitem([testAccount3.name, groupName], {from: testAccount2});

        //call mintitem() on marble contract
        // const res = await marbleContract.actions.mintitem([toAccount, groupName], {from: testAccount2});
        // assert(res.processed.receipt.status == 'executed', "mintitem() action was not executed");

        //call transferitem() on marble contract
        const res = await marbleContract.actions.transferitem([fromAccount, toAccount, serials, memo], {from: testAccount3});
        assert(res.processed.receipt.status == 'executed', "transferitem() action was not executed");

        //assert items table values
        const itemsTable = await marbleContract.provider.select('items').from('mbl').range(1, 2).limit(2).find();
        assert(itemsTable[0].serial == 1, "Incorrect Item Serial");
        assert(itemsTable[0].group == groupName, "Incorrect Item Group");
        assert(itemsTable[0].owner == toAccount, "Incorrect Item Owner");

        assert(itemsTable[1].serial == 2, "Incorrect Item Serial");
        assert(itemsTable[1].group == groupName, "Incorrect Item Group");
        assert(itemsTable[1].owner == toAccount, "Incorrect Item Owner");
    });

    it("Activate Item", async () => {
        //initialize
        const groupName = "heroes";
        const behaviorName = "activate";
        const serial = 1;

        //call togglebhvr() on marble contract
        await marbleContract.actions.togglebhvr([groupName, behaviorName], {from: testAccount2});

        //call activateitem() on marble contract
        const res = await marbleContract.actions.activateitem([serial], {from: testAccount1});
        assert(res.processed.receipt.status == 'executed', "activateitem() action was not executed");
    });

    it("Reclaim Item", async () => {
        //initialize
        const groupName = "heroes";
        const behaviorName = "reclaim";
        const serial = 1;

        //call togglebhvr() on marble contract
        await marbleContract.actions.togglebhvr([groupName, behaviorName], {from: testAccount2});

        //call reclaimitem() on marble contract
        const res = await marbleContract.actions.reclaimitem([serial], {from: testAccount2});
        assert(res.processed.receipt.status == 'executed', "reclaimitem() action was not executed");

        //assert items table values
        const itemsTable = await marbleContract.provider.select('items').from('mbl').equal(serial).find();
        assert(itemsTable[0].serial == serial, "Incorrect Item Serial");
        assert(itemsTable[0].group == groupName, "Incorrect Item Group");
        assert(itemsTable[0].owner == testAccount2.name, "Incorrect Item Owner");
    });

    it("Consume Item", async () => {
        //initialize
        const groupName = "heroes";
        const behaviorName = "consume";
        const serial = 2;

        //call togglebhvr() on marble contract
        await marbleContract.actions.togglebhvr([groupName, behaviorName], {from: testAccount2});

        //call consumeitem() on marble contract
        const res = await marbleContract.actions.consumeitem([serial], {from: testAccount1});
        assert(res.processed.receipt.status == 'executed', "consumeitem() action was not executed");

        //assert items table values
        const itemsTable = await marbleContract.provider.select('items').from('mbl').equal(serial).find();
        assert(itemsTable.length == 0, "Item Not Consumed");

        //assert groups table values
        const groupsTable = await marbleContract.provider.select('groups').from('mbl').find('heroes');
        assert(groupsTable[0].supply == 1, "Incorrect Supply");
        assert(groupsTable[0].issued_supply == 2, "Incorrect Issued Supply");
    });

    it("Destroy Item", async () => {
        //initialize
        const groupName = "heroes";
        const behaviorName = "consume";
        const serial = 1;
        const memo = "";

        //call destroyitem() on marble contract
        const res = await marbleContract.actions.destroyitem([serial, memo], {from: testAccount2});
        assert(res.processed.receipt.status == 'executed', "destroyitem() action was not executed");

        //assert items table values
        const itemsTable = await marbleContract.provider.select('items').from('mbl').equal(serial).find();
        assert(itemsTable.length == 0, "Item Not Destroyed");

        //assert groups table values
        const groupsTable = await marbleContract.provider.select('groups').from('mbl').find('heroes');
        assert(groupsTable[0].supply == 0, "Incorrect Supply");
        assert(groupsTable[0].issued_supply == 2, "Incorrect Issued Supply");
    });

    
    
});