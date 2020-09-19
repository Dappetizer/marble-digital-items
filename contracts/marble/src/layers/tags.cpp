//======================== tag actions ========================

ACTION marble::newtag(uint64_t serial, name tag_name, string content, optional<string> checksum, optional<string> algorithm, bool shared)
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
    check(tag_name != name(0), "tag name cannot be empty");

    //initialize
    string chsum = (checksum) ? *checksum : "";
    string algo = (algorithm) ? *algorithm : "";

    //if shared tag
    if (shared) {
        //open shared tags table, find shared tag
        shared_tags_table shared_tags(get_self(), grp.group_name.value);
        auto shared_tag_itr = shared_tags.find(tag_name.value);

        //validate
        check(shared_tag_itr == shared_tags.end(), "shared tag name already exists");

        //emplace shared tag
        //ram payer: self
        shared_tags.emplace(get_self(), [&](auto& col) {
            col.tag_name = tag_name;
            col.content = content;
            col.checksum = chsum;
            col.algorithm = algo;
            col.locked = false;
        });
    } else {
        //open tags table, find tag
        tags_table tags(get_self(), serial);
        auto tag_itr = tags.find(tag_name.value);

        //validate
        check(tag_itr == tags.end(), "tag name already exists on item");

        //emplace tag
        //ram payer: self
        tags.emplace(get_self(), [&](auto& col) {
            col.tag_name = tag_name;
            col.content = content;
            col.checksum = chsum;
            col.algorithm = algo;
            col.locked = false;
        });
    }
}

ACTION marble::updatetag(uint64_t serial, name tag_name, string new_content, optional<string> new_checksum, optional<string> new_algorithm, bool shared)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    string new_chsum = (new_checksum) ? *new_checksum : "";
    string new_algo = (new_algorithm) ? *new_algorithm : "";

    // if (new_checksum) {
    //     new_chsum = *new_checksum;
    // }

    // if (new_algorithm) {
    //     new_algo = *new_algorithm;
    // }

    //if shared tag
    if (shared) {
        //open shared tags table, get shared tag
        shared_tags_table shared_tags(get_self(), grp.group_name.value);
        auto& st = shared_tags.get(tag_name.value, "shared tag not found");

        //validate
        check(!st.locked, "shared tag is locked");

        //update shared tag
        shared_tags.modify(st, same_payer, [&](auto& col) {
            col.content = new_content;
            col.checksum = new_chsum;
            col.algorithm = new_algo;
        });
    } else {
        //open tags table, get tag
        tags_table tags(get_self(), serial);
        auto& t = tags.get(tag_name.value, "tag not found on item");

        //validate
        check(!t.locked, "tag is locked");

        //update tag
        tags.modify(t, same_payer, [&](auto& col) {
            col.content = new_content;
            col.checksum = new_chsum;
            col.algorithm = new_algo;
        });
    }
}

ACTION marble::locktag(uint64_t serial, name tag_name, bool shared)
{
    //open items table, get item
    items_table items(get_self(), get_self().value);
    auto& itm = items.get(serial, "item not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(itm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //if shared tag
    if (shared) {
        //open shared tags table, get shared tag
        shared_tags_table shared_tags(get_self(), grp.group_name.value);
        auto& st = shared_tags.get(tag_name.value, "shared tag not found");

        //validate
        check(!st.locked, "shared tag is already locked");

        //modify shared tag
        shared_tags.modify(st, same_payer, [&](auto& col) {
            col.locked = true;
        });
    } else {
        //open tags table, get tag
        tags_table tags(get_self(), serial);
        auto& t = tags.get(tag_name.value, "tag not found on item");

        //validate
        check(!t.locked, "tag is already locked");

        //modify tag
        tags.modify(t, same_payer, [&](auto& col) {
            col.locked = true;
        });
    }
}

ACTION marble::rmvtag(uint64_t serial, name group_name, name tag_name, string memo, bool shared)
{
    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(group_name.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //if shared tag
    if (shared) {
        //open shared tags table, get shared tag
        shared_tags_table shared_tags(get_self(), grp.group_name.value);
        auto& st = shared_tags.get(tag_name.value, "shared tag not found");

        //remove tag
        shared_tags.erase(st);
    } else {
        //open tags table, get tag
        tags_table tags(get_self(), serial);
        auto& t = tags.get(tag_name.value, "tag not found on item");

        //remove item
        tags.erase(t);
    }
}
