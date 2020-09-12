//======================== attribute actions ========================

ACTION marble::newattribute(uint64_t serial, name attribute_name, int64_t initial_points, bool shared)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //if shared attribute
    if (shared) {
        //open shared attributes table, find shared attribute
        shared_attributes_table shared_attributes(get_self(), grp.group_name.value);
        auto sh_attr_itr = shared_attributes.find(attribute_name.value);

        //validate
        check(sh_attr_itr == shared_attributes.end(), "shared attributes already exists");

        //create new shared attribute
        //ram payer: contract
        shared_attributes.emplace(get_self(), [&](auto& col) {
            col.attribute_name = attribute_name;
            col.points = initial_points;
            col.locked = false;
        });
    } else {
        //open attributes table, find attribute
        attributes_table attributes(get_self(), serial);
        auto attr_itr = attributes.find(attribute_name.value);

        //validate
        check(attr_itr == attributes.end(), "attribute name already exists for item");

        //create new attribute
        //ram payer: contract
        attributes.emplace(get_self(), [&](auto& col) {
            col.attribute_name = attribute_name;
            col.points = initial_points;
            col.locked = false;
        });
    }
}

ACTION marble::setpoints(uint64_t serial, name attribute_name, int64_t new_points, bool shared)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //if shared attribute
    if (shared) {
        //open shared attributes table, get shared attribute
        shared_attributes_table shared_attributes(get_self(), grp.group_name.value);
        auto& sh_attr = shared_attributes.get(attribute_name.value, "shared attribute not found");

        //validate
        check(!sh_attr.locked, "shared attribute is locked");

        //update shared attribute
        shared_attributes.modify(sh_attr, same_payer, [&](auto& col) {
            col.points = new_points;
        });
    } else {
        //open attributes table, get attribute
        attributes_table attributes(get_self(), serial);
        auto& attr = attributes.get(attribute_name.value, "attribute not found");

        //validate
        check(!attr.locked, "attribute is locked");

        //update attribute
        attributes.modify(attr, same_payer, [&](auto& col) {
            col.points = new_points;
        });
    }
}

ACTION marble::increasepts(uint64_t serial, name attribute_name, uint64_t points_to_add, bool shared)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //validate
    check(points_to_add > 0, "points to add must be greater than zero");

    //if shared attribute
    if (shared) {
        //open shared attributes table, get shared attribute
        shared_attributes_table shared_attributes(get_self(), grp.group_name.value);
        auto& sh_attr = shared_attributes.get(attribute_name.value, "shared attribute not found");

        //validate
        check(!sh_attr.locked, "shared attribute is locked");
        
        //update shared attribute
        shared_attributes.modify(sh_attr, same_payer, [&](auto& col) {
            col.points += points_to_add;
        });
    } else {
        //open attributes table, get attribute
        attributes_table attributes(get_self(), serial);
        auto& attr = attributes.get(attribute_name.value, "attribute not found");

        //validate
        check(!attr.locked, "attribute is locked");

        //update attribute
        attributes.modify(attr, same_payer, [&](auto& col) {
            col.points += points_to_add;
        });
    }
}

ACTION marble::decreasepts(uint64_t serial, name attribute_name, uint64_t points_to_subtract, bool shared)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //validate
    check(points_to_subtract > 0, "must subtract greater than zero points");

    //if shared attribute
    if (shared) {
        //open shared attributes table, get shared attribute
        shared_attributes_table shared_attributes(get_self(), grp.group_name.value);
        auto& sh_attr = shared_attributes.get(attribute_name.value, "shared attribute not found");

        //validate
        check(!sh_attr.locked, "shared attribute is locked");

        //udpate shared attribute
        shared_attributes.modify(sh_attr, same_payer, [&](auto& col) {
            col.points -= points_to_subtract;
        });
    } else {
        //open attributes table, get attribute
        attributes_table attributes(get_self(), serial);
        auto& attr = attributes.get(attribute_name.value, "attribute not found");

        //validate
        check(!attr.locked, "attribute is locked");

        //update attribute
        attributes.modify(attr, same_payer, [&](auto& col) {
            col.points -= points_to_subtract;
        });
    }
}

ACTION marble::lockattr(uint64_t serial, name attribute_name, bool shared)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //if shared attribute
    if (shared) {
        //open shared attributes table, get shared attribute
        shared_attributes_table shared_attributes(get_self(), grp.group_name.value);
        auto& sh_attr = shared_attributes.get(attribute_name.value, "shared attribute not found");

        //validate
        check(!sh_attr.locked, "shared attribute is already locked");

        //update shared attribute
        shared_attributes.modify(sh_attr, same_payer, [&](auto& col) {
            col.locked = true;
        });
    } else {
        //open attributes table, get attribute
        attributes_table attributes(get_self(), serial);
        auto& attr = attributes.get(attribute_name.value, "attribute not found");

        //validate
        check(!attr.locked, "attribute is already locked");

        //update attribute
        attributes.modify(attr, same_payer, [&](auto& col) {
            col.locked = true;
        });
    }
}

ACTION marble::rmvattribute(uint64_t serial, name group_name, name attribute_name, bool shared)
{
    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(group_name.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //if shared attribute
    if (shared) {
        //open shared attributes table, get shared attribute
        shared_attributes_table shared_attributes(get_self(), grp.group_name.value);
        auto& sh_attr = shared_attributes.get(attribute_name.value, "shared attribute not found");

        //remove shared attribute
        shared_attributes.erase(sh_attr);
    } else {
        //open attributes table, get attribute
        attributes_table attributes(get_self(), serial);
        auto& attr = attributes.get(attribute_name.value, "attribute not found");

        //remove attribute
        attributes.erase(attr);
    }
}
