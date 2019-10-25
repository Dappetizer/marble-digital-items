<h1 class="contract">init</h1>
Initialize the Marble contract.

### Parameters

* **initial_version** (initial version number)

* **initial_access** (initial contract access level)

### Intent

Initialize the Marble contract config with the given initial version and acess level.

### Body

...

<h1 class="contract">setversion</h1>
Sets a new version number for the contract.

### Parameters

* **new_version** (new version number)

### Intent

Change the current version to the new version.

### Body

...

<h1 class="contract">setadmin</h1>
Sets a new contract admin.

### Parameters

* **new_admin** (new admin name)

### Intent

Change the current admin to the new admin.

### Body

...

<h1 class="contract">setaccess</h1>
Sets a new contract access level.

### Parameters

* **new_access** (new access name)

### Intent

Change the current access level to the new access.

### Body

...

<h1 class="contract">newcollectn</h1>
Define a new Collection.

### Parameters

* **title** (collection title)

* **description** (collection description)

* **collection_name** (name of collection)

* **manager** (name of collection manager)

* **supply_cap** (number of nfts allowed in collection)

### Intent

Create a new collection with the given parameters.

### Body

...

<h1 class="contract">addoption</h1>
Add a new option to a collection.

### Parameters

* **collection_name** (name of the collection to add the option to)

* **option_name** (name of the option to add)

* **initial_value** (the initial value of the option)

### Intent

Add a new option to a collection.

### Body

...

<h1 class="contract">toggle</h1>
Toggle a collection option on or off.

### Parameters

* **collection_name** (name of the collection with the option to toggle)

* **option_name** (the name of the option to toggle)

* **memo** (a memo describing the toggle)

### Intent
Toggle a collection option on or off.

### Body

...

<h1 class="contract">rmvoption</h1>
Removes an option from a collection.

### Parameters

* **collection_name** (name of the collection with the option to toggle)

* **option_name** (the name of the option to toggle)

### Intent
Removes an option form a collection.

### Body

...

<h1 class="contract">setmanager</h1>
Sets a new collection manager

### Parameters

* **collection_name** (name of the collection with the option to toggle)

* **new_manager** (name of the new manager)

* **memo** (a memo describing the change of management)

### Intent
Sets a new collection manager.

### Body

...

<h1 class="contract">newnft</h1>
Creates a new nft.

### Parameters

* **owner** (name of the account that will own the nft)

* **collection_name** (the name of the collcetion the nft belongs to)

* **content** (raw content or a link to content)

* **checksum** (checksum of content)

* **algorithm** (algorithm used to produce checksum)

### Intent
Creates a new nft.

### Body

...

<h1 class="contract">updatenft</h1>
Updates an NFT's content.

### Parameters

* **serial** (serial number of nft to update)

* **content** (new raw content or link to content)

* **checksum** (checksum of new content)

* **algorithm** (algorithm used to produce new checksum)

### Intent
Updates an NFT's content.

### Body

...

<h1 class="contract">transfernft</h1>
Transfers ownership of an NFT to a new account.

### Parameters

* **serial** (serial number of nft to transfer)

* **new_owner** (name of the new owner account)

* **memo** (memo describing the transfer)

### Intent
Transfers ownership of an NFT to a new account.

### Body

...

<h1 class="contract">destroynft</h1>
Destroys an NFT and removes it from circulation.

### Parameters

* **serial** (serial number of nft to destroy)

* **memo** (memo describing the nft destruction)

### Intent
Destroys an NFT and removes it from circulation.

### Body

...

<h1 class="contract">newattribute</h1>
Assigns an NFT a new attribute.

### Parameters

* **serial** (serial number of the nft receiving the attribute)

* **attribute_name** (name of the attribute to assign)

* **initial_points** (initial points assigned to attribute)

### Intent
Assigns an NFT a new attribute.

### Body

...

<h1 class="contract">setpoints</h1>
Sets the point value of an NFT attribute.

### Parameters

* **serial** (serial number of the nft)

* **attribute_name** (name of the attribute to set points to)

* **new_points** (new point value to set to the attribute)

### Intent
Sets the point value of an NFT attribute.

### Body

...

<h1 class="contract">increasepts</h1>
Increases an NFT attribute's points by a given amount.

### Parameters

* **serial** (serial number of the nft)

* **attribute_name** (name of the attribute to add points to)

* **points_to_add** (amount of points to add)

### Intent
Increases an NFT attribute's points by a given amount.

### Body

...

<h1 class="contract">decreasepts</h1>
Decreases an NFT attribute's points by a given amount.

### Parameters

* **serial** (serial number of the nft)

* **attribute_name** (name of the attribute to subtract points from)

* **points_to_subtract** (amount of points to subtract)

### Intent
Decreases an NFT attribute's points by a given amount.

### Body

...

<h1 class="contract">rmvattribute</h1>
Removes an attribute from and NFT.

### Parameters

* **serial** (serial number of the nft)

* **attribute_name** (name of the attribute to remove)

### Intent
Removes an attribute from and NFT.

### Body

...
