#include <common.hpp>
#include <document_graph/document_graph.hpp>
#include <document_graph/document.hpp>
#include <document_graph/content.hpp>
#include <document_graph/edge.hpp>
#include <document_graph/util.hpp>
#include <migration.hpp>
#include <member.hpp>
#include <util.hpp>

namespace hypha
{
    Migration::Migration(dao &dao) : m_dao{dao} {}

    void Migration::migrateRole(const uint64_t &roleId)
    {
        eosio::name scope = eosio::name("role");

        Migration::object_table o_t_role(m_dao.get_self(), scope.value);
        auto o_itr_role = o_t_role.find(roleId);
        eosio::check(o_itr_role != o_t_role.end(), "roleId does not exist: " + std::to_string(roleId));

        // create the new document
        Document roleDocument = newDocument(o_itr_role->id,
                                            scope,
                                            o_itr_role->created_date,
                                            o_itr_role->names,
                                            o_itr_role->strings,
                                            o_itr_role->assets,
                                            o_itr_role->time_points,
                                            o_itr_role->ints);

        //  member          ---- owns       ---->   roleDocument
        //  roleDocument    ---- ownedBy    ---->   member

        auto owner = hypha::Member::get(m_dao.get_self(), o_itr_role->names.at("owner"));

        Edge::write(m_dao.get_self(), m_dao.get_self(), owner.getHash(), roleDocument.getHash(), common::OWNS);
        Edge::write(m_dao.get_self(), m_dao.get_self(), roleDocument.getHash(), owner.getHash(), common::OWNED_BY);
        Edge::write(m_dao.get_self(), m_dao.get_self(), getRoot(m_dao.get_self()), roleDocument.getHash(), common::ROLE_NAME);

        // add the cross-reference to our temporary migration lookup table
        XReferenceTable xrt(m_dao.get_self(), m_dao.get_self().value);
        xrt.emplace(m_dao.get_self(), [&](auto &x) {
            x.id = xrt.available_primary_key();
            x.object_scope = scope;
            x.object_id = o_itr_role->id;
            x.document_hash = roleDocument.getHash();
        });

        // erase the table record
        o_t_role.erase(o_itr_role);
    }

    void Migration::addMemberToTable(const eosio::name &member)
    {
        Migration::member_table m_t(m_dao.get_self(), m_dao.get_self().value);
        if (m_t.find(member.value) != m_t.end())
        {
            return;
        }

        m_t.emplace(m_dao.get_self(), [&](auto &m) {
            m.member = member;
        });
    }

    void Migration::addApplicant(const eosio::name &applicant, const std::string content)
    {
        Migration::applicant_table a_t(m_dao.get_self(), m_dao.get_self().value);
        if (a_t.find(applicant.value) != a_t.end())
        {
            return;
        }

        a_t.emplace(m_dao.get_self(), [&](auto &a) {
            a.applicant = applicant;
            a.content = content;
        });
    }

    int Migration::defSetSetting(const string &key, const Content::FlexValue &value, int senderId)
    {
        eosio::transaction out{};
        out.actions.emplace_back(
            eosio::permission_level{m_dao.get_self(), name("active")},
            m_dao.get_self(), name("setsetting"),
            make_tuple(key, value));

        out.delay_sec = 1;
        out.send(senderId, m_dao.get_self());
        return senderId;
    }

    void Migration::migrateConfig()
    {
        config_table config_s(m_dao.get_self(), m_dao.get_self().value);
        Config c = config_s.get_or_create(m_dao.get_self(), Config());
        int senderId = eosio::current_time_point().sec_since_epoch();

        std::map<string, name>::const_iterator name_itr;
        for (name_itr = c.names.begin(); name_itr != c.names.end(); ++name_itr)
        {
            defSetSetting(name_itr->first, name_itr->second, ++senderId);
        }

        std::map<string, asset>::const_iterator asset_itr;
        for (asset_itr = c.assets.begin(); asset_itr != c.assets.end(); ++asset_itr)
        {
            defSetSetting(asset_itr->first, asset_itr->second, ++senderId);
        }

        std::map<string, string>::const_iterator string_itr;
        for (string_itr = c.strings.begin(); string_itr != c.strings.end(); ++string_itr)
        {
            defSetSetting(string_itr->first, string_itr->second, ++senderId);
        }

        std::map<string, uint64_t>::const_iterator int_itr;
        for (int_itr = c.ints.begin(); int_itr != c.ints.end(); ++int_itr)
        {
            defSetSetting(int_itr->first, int_itr->second, ++senderId);
        }

        std::map<string, time_point>::const_iterator time_point_itr;
        for (time_point_itr = c.time_points.begin(); time_point_itr != c.time_points.end(); ++time_point_itr)
        {
            defSetSetting(time_point_itr->first, time_point_itr->second, ++senderId);
        }
        config_s.remove();
    }

    void Migration::migrateMember(const eosio::name &memberName)
    {
        eosio::require_auth(m_dao.get_self());

        // create the member document
        hypha::Member member(m_dao.get_self(), m_dao.get_self(), memberName);
        eosio::checksum256 root = getRoot(m_dao.get_self());

        // create the new member edges
        Edge::write(m_dao.get_self(), m_dao.get_self(), root, member.getHash(), common::MEMBER);
        Edge::write(m_dao.get_self(), m_dao.get_self(), member.getHash(), root, common::MEMBER_OF);

        Migration::member_table m_t(m_dao.get_self(), m_dao.get_self().value);
        auto m_itr = m_t.find(memberName.value);
        eosio::check(m_itr != m_t.end(), "member not in members table: " + memberName.to_string());
        m_t.erase(m_itr);
    }

    void Migration::eraseAllObjects(const eosio::name &scope)
    {
        Migration::object_table o_t(m_dao.get_self(), scope.value);
        auto o_itr = o_t.begin();
        while (o_itr != o_t.end())
        {
            o_itr = o_t.erase(o_itr);
        }
    }

    void Migration::eraseAll(bool skipPeriods)
    {
        eosio::require_auth(m_dao.get_self());

        Edge::edge_table e_t(m_dao.get_self(), m_dao.get_self().value);
        auto e_itr = e_t.begin();
        while (e_itr != e_t.end())
        {
            e_itr = e_t.erase(e_itr);
        }

        Document::document_table d_t(m_dao.get_self(), m_dao.get_self().value);
        auto d_itr = d_t.begin();
        while (d_itr != d_t.end())
        {
            d_itr = d_t.erase(d_itr);
        }

        // dao::PeriodTable p_t(m_dao.get_self(), m_dao.get_self().value);
        // auto p_itr = p_t.begin();
        // while (p_itr != p_t.end())
        // {
        //     p_itr = p_t.erase(p_itr);
        // }
    }

    void Migration::reset4test()
    {
        eosio::require_auth(m_dao.get_self());

        Document settingsDocument = m_dao.getSettingsDocument();
        ContentWrapper cw = settingsDocument.getContentWrapper();
        ContentGroup *settings = cw.getGroupOrFail("settings");

        eraseAll(true);

        ContentGroups newSettingsCGS{
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, SYSTEM),
                Content(TYPE, common::SETTINGS_EDGE),
                Content(NODE_LABEL, "Settings")},
            *settings};

        Document updatedSettings(m_dao.get_self(), m_dao.get_self(), newSettingsCGS);
        Document rootDocument(m_dao.get_self(), m_dao.get_self(), getRootContent(m_dao.get_self()));
        Edge::write(m_dao.get_self(), m_dao.get_self(), rootDocument.getHash(), updatedSettings.getHash(), common::SETTINGS_EDGE);

        m_dao.setSetting(ROOT_NODE, rootDocument.getHash());
    }

    Document Migration::newDocument(const uint64_t id,
                                    const name scope,
                                    const time_point createdDate,
                                    const map<string, name> names,
                                    const map<string, string> strings,
                                    const map<string, asset> assets,
                                    const map<string, time_point> time_points,
                                    const map<string, uint64_t> ints)
    {
        ContentGroups contentGroups{};

        ContentGroup systemContentGroup{};
        systemContentGroup.push_back(Content(CONTENT_GROUP_LABEL, SYSTEM));
        systemContentGroup.push_back(Content("legacy_object_scope", scope));
        systemContentGroup.push_back(Content("legacy_object_id", id));
        systemContentGroup.push_back(Content("legacy_object_created_date", createdDate));

        ContentGroup detailsContentGroup{};
        detailsContentGroup.push_back(Content(CONTENT_GROUP_LABEL, DETAILS));

        std::map<string, name>::const_iterator name_itr;
        for (name_itr = names.begin(); name_itr != names.end(); ++name_itr)
        {
            Content content{name_itr->first, name_itr->second};
            if (name_itr->first == "trx_action_contract" ||
                name_itr->first == "trx_action_name" ||
                name_itr->first == "prior_scope")
            {
                // skip
            }
            else if (name_itr->first == "type" ||
                     name_itr->first == "ballot_id")
            {
                // add to SYSTEM group
                systemContentGroup.push_back(content);
            }
            else
            {
                // add to DETAILS group
                detailsContentGroup.push_back(content);
            }
        }

        std::map<string, asset>::const_iterator asset_itr;
        for (asset_itr = assets.begin(); asset_itr != assets.end(); ++asset_itr)
        {
            // all assets go to the details group
            detailsContentGroup.push_back(Content{asset_itr->first, asset_itr->second});
        }

        std::map<string, string>::const_iterator string_itr;
        for (string_itr = strings.begin(); string_itr != strings.end(); ++string_itr)
        {
            Content content{string_itr->first, string_itr->second};
            if (string_itr->first == "client_version" ||
                string_itr->first == "contract_version")
            {
                // add to SYSTEM group
                systemContentGroup.push_back(content);
            }
            else
            {
                if (string_itr->first == "title") // use the role's title as the node label
                {
                    systemContentGroup.push_back(Content(NODE_LABEL, string_itr->second));
                }
                // add to DETAILS group
                detailsContentGroup.push_back(content);
            }
        }

        std::map<string, uint64_t>::const_iterator int_itr;
        for (int_itr = ints.begin(); int_itr != ints.end(); ++int_itr)
        {
            Content content{int_itr->first, int_itr->second};
            if (int_itr->first == "object_id" ||
                int_itr->first == "prior_id")
            {
                // skip
            }
            else if (false)
            {
                // add to SYSTEM group
                systemContentGroup.push_back(content);
            }
            else
            {
                // add to DETAILS group
                detailsContentGroup.push_back(content);
            }
        }

        std::map<string, time_point>::const_iterator time_point_itr;
        for (time_point_itr = time_points.begin(); time_point_itr != time_points.end(); ++time_point_itr)
        {
            Content content{time_point_itr->first, time_point_itr->second};
            if (true)
            {
                // skip
            }
            else if (false)
            {
                // add to SYSTEM group
                systemContentGroup.push_back(content);
            }
            else
            {
                // add to DETAILS group
                detailsContentGroup.push_back(content);
            }
        }

        contentGroups.push_back(systemContentGroup);
        contentGroups.push_back(detailsContentGroup);

        return Document(m_dao.get_self(), m_dao.get_self(), contentGroups);
    }

    void Migration::newObject(const uint64_t &id,
                              const name &scope,
                              map<string, name> names,
                              map<string, string> strings,
                              map<string, asset> assets,
                              map<string, time_point> time_points,
                              map<string, uint64_t> ints)
    {

        eosio::print("writing new object: " + std::to_string(id));
        Migration::object_table o_t(m_dao.get_self(), scope.value);
        o_t.emplace(m_dao.get_self(), [&](auto &o) {
            o.id = id;
            o.names = names;
            o.strings = strings;
            o.assets = assets;
            o.time_points = time_points;
            o.ints = ints;
        });
    }

} // namespace hypha