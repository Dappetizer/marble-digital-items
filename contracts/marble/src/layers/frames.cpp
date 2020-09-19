//======================== frame actions ========================

ACTION marble::newframe(name frame_name, name group, map<name, string> default_tags, map<name, int64_t> default_attributes)
{
    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open frames table, find frame
    frames_table frames(get_self(), get_self().value);
    auto frm_itr = frames.find(frame_name.value);

    //validate
    check(frm_itr == frames.end(), "frame already exists");

    //emplace new frame
    //ram payer: self
    frames.emplace(get_self(), [&](auto& col) {
        col.frame_name = frame_name;
        col.group = group;
        col.default_tags = default_tags;
        col.default_attributes = default_attributes;
    });
}

ACTION marble::applyframe(name frame_name, uint64_t serial, bool overwrite)
{
    //open frames table, get frame
    frames_table frames(get_self(), get_self().value);
    auto& frm = frames.get(frame_name.value, "frame not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(frm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //apply default tags
    for (auto itr = frm.default_tags.begin(); itr != frm.default_tags.end(); itr++) {
        //open tags table, find tag
        tags_table tags(get_self(), serial);
        auto tg_itr = tags.find(itr->first.value);

        //NOTE: will skip existing tag with same tag name if overwrite is false

        //if tag not found
        if (tg_itr == tags.end()) {
            //emplace new tag
            //ram payer: self
            tags.emplace(get_self(), [&](auto& col) {
                col.tag_name = itr->first;
                col.content = itr->second;
                col.checksum = "";
                col.algorithm = "";
            });
        } else if (overwrite) {
            //validate
            check(!tg_itr->locked, "tag is locked");

            //overwrite existing tag
            tags.modify(tg_itr, same_payer, [&](auto& col) {
                col.content = itr->second;
                col.checksum = "";
                col.algorithm = "";
            });
        }
    }

    //apply default attributes
    for (auto itr = frm.default_attributes.begin(); itr != frm.default_attributes.end(); itr++) {
        //open attributes table, find attribute
        attributes_table attributes(get_self(), serial);
        auto attr_itr = attributes.find(itr->first.value);

        //NOTE: will skip existing attribute with same attribute name if overwrite is false

        //if attribute not found
        if (attr_itr == attributes.end()) {
            //emplace new attribute
            //ram payer: self
            attributes.emplace(get_self(), [&](auto& col) {
                col.attribute_name = itr->first;
                col.points = itr->second;
            });
        } else if (overwrite) {
            //validate
            check(!attr_itr->locked, "attribute is locked");

            //overwrite existing attribute
            attributes.modify(attr_itr, same_payer, [&](auto& col) {
                col.points = itr->second;
            });
        }
    }
}

ACTION marble::quickbuild(name frame_name, name to, map<name, string> override_tags, map<name, int64_t> override_attributes)
{
    //open frames table, get frame
    frames_table frames(get_self(), get_self().value);
    auto& frm = frames.get(frame_name.value, "frame not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(frm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //open config table, get config
    config_table configs(get_self(), get_self().value);
    auto conf = configs.get();

    //initialize
    uint64_t item_serial = conf.last_serial + 1;

    //queue inline mintitem()
    action(permission_level{get_self(), name("active")}, get_self(), name("mintitem"), make_tuple(
        to, //to
        frm.group //group_name
    )).send();

    //apply default tags
    for (auto itr = frm.default_tags.begin(); itr != frm.default_tags.end(); itr++) {
        //open tags table, search for tag
        tags_table tags(get_self(), item_serial);
        auto tg_itr = tags.find(itr->first.value);

        //if tag not found
        if (tg_itr == tags.end()) {
            //emplace new tag
            //ram payer: contract
            tags.emplace(get_self(), [&](auto& col) {
                col.tag_name = itr->first;
                col.content = itr->second;
                col.checksum = "";
                col.algorithm = "";
            });
        } else {
            //overwrite existing tag
            tags.modify(tg_itr, same_payer, [&](auto& col) {
                col.content = itr->second;
                col.checksum = "";
                col.algorithm = "";
            });
        }
    }

    //apply default attributes
    for (auto itr = frm.default_attributes.begin(); itr != frm.default_attributes.end(); itr++) {
        //open attributes table, find attribute
        attributes_table attributes(get_self(), item_serial);
        auto attr_itr = attributes.find(itr->first.value);

        //if attribute not found
        if (attr_itr == attributes.end()) {
            //emplace new attribute
            //ram payer: contract
            attributes.emplace(get_self(), [&](auto& col) {
                col.attribute_name = itr->first;
                col.points = itr->second;
            });
        } else {

            //overwrite existing attribute
            attributes.modify(attr_itr, same_payer, [&](auto& col) {
                col.points = itr->second;
            });
        }
    }

    //apply tag overrides
    for (auto itr = override_tags.begin(); itr != override_tags.end(); itr++) {
        //open tags table, search for tag
        tags_table tags(get_self(), item_serial);
        auto tg_itr = tags.find(itr->first.value);

        //if tag not found
        if (tg_itr == tags.end()) {
            //emplace new tag
            //ram payer: contract
            tags.emplace(get_self(), [&](auto& col) {
                col.tag_name = itr->first;
                col.content = itr->second;
                col.checksum = "";
                col.algorithm = "";
            });
        } else {
            //overwrite existing tag
            tags.modify(tg_itr, same_payer, [&](auto& col) {
                col.content = itr->second;
                col.checksum = "";
                col.algorithm = "";
            });
        }
    }

    //apply attribute overrides
    for (auto itr = override_attributes.begin(); itr != override_attributes.end(); itr++) {
        //open attributes table, find attribute
        attributes_table attributes(get_self(), item_serial);
        auto attr_itr = attributes.find(itr->first.value);

        //if attribute not found
        if (attr_itr == attributes.end()) {
            
            //emplace new attribute
            //ram payer: self
            attributes.emplace(get_self(), [&](auto& col) {
                col.attribute_name = itr->first;
                col.points = itr->second;
            });
        } else {
            //overwrite existing attribute
            attributes.modify(attr_itr, same_payer, [&](auto& col) {
                col.points = itr->second;
            });
        }
    }
}

ACTION marble::cleanframe(name frame_name, uint64_t serial)
{
    //open frames table, get frame
    frames_table frames(get_self(), get_self().value);
    auto& frm = frames.get(frame_name.value, "frame not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(frm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //clean default tags
    for (auto itr = frm.default_tags.begin(); itr != frm.default_tags.end(); itr++) {
        //open tags table, find tag
        tags_table tags(get_self(), serial);
        auto tag_itr = tags.find(itr->first.value);

        //if tag found
        if (tag_itr != tags.end()) {
            //delete tag
            tags.erase(*tag_itr);
        }
    }

    //clean default attributes
    for (auto itr = frm.default_attributes.begin(); itr != frm.default_attributes.end(); itr++) {
        //open attributes table, find attribute
        attributes_table attributes(get_self(), serial);
        auto attr_itr = attributes.find(itr->first.value);

        //if attribute found
        if (attr_itr != attributes.end()) {
            //delete attribute
            attributes.erase(*attr_itr);
        }
    }

    //clean default events
    // for (auto itr = frm.default_events.begin(); itr != frm.default_events.end(); itr++) {
    //     //open events table, find event
    //     events_table events(get_self(), serial);
    //     auto event_itr = events.find(itr->first.value);

    //     //if event found
    //     if (event_itr == events.end()) {
    //         //delete event
    //         events.erase(*event_itr);
    //     }
    // }
}

ACTION marble::rmvframe(name frame_name, string memo)
{
    //open frames table, get frame
    frames_table frames(get_self(), get_self().value);
    auto& frm = frames.get(frame_name.value, "frame not found");

    //open groups table, get group
    groups_table groups(get_self(), get_self().value);
    auto& grp = groups.get(frm.group.value, "group not found");

    //authenticate
    require_auth(grp.manager);

    //erase frame
    frames.erase(frm);
}
