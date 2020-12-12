#include <common.hpp>
#include <document_graph/document_graph.hpp>
#include <document_graph/document.hpp>
#include <document_graph/content.hpp>
#include <document_graph/edge.hpp>
#include <document_graph/util.hpp>
#include <migration.hpp>
#include <member.hpp>

namespace hypha
{
    Migration::Migration(const eosio::name contract) : m_contract{contract} {}

    void Migration::migrateRole(const uint64_t &roleId)
    {
        eosio::name scope = eosio::name("role");

        Migration::object_table o_t_role(m_contract, scope.value);
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
        Edge::write(m_contract, m_contract, Member::getHash(o_itr_role->names.at("owner")), roleDocument.getHash(), common::OWNS);
        Edge::write(m_contract, m_contract, roleDocument.getHash(), Member::getHash(o_itr_role->names.at("owner")), common::OWNED_BY);

        // add the cross-reference to our temporary migration lookup table
        XReferenceTable xrt(m_contract, m_contract.value);
        xrt.emplace(m_contract, [&](auto &x) {
            x.id = xrt.available_primary_key();
            x.object_scope = scope;
            x.object_id = o_itr_role->id;
            x.document_hash = roleDocument.getHash();
        });

        // erase the table record
        o_t_role.erase(o_itr_role);
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

        return Document(m_contract, m_contract, contentGroups);
    }

    void Migration::newObject(const uint64_t &id,
                              const name &scope,
                              map<string, name> names,
                              map<string, string> strings,
                              map<string, asset> assets,
                              map<string, time_point> time_points,
                              map<string, uint64_t> ints)
    {

        eosio::print ("writing new object: " + std::to_string(id));
        Migration::object_table o_t(m_contract, scope.value);
        o_t.emplace(m_contract, [&](auto &o) {
            o.id = id;
            o.names = names;
            o.strings = strings;
            o.assets = assets;
            o.time_points = time_points;
            o.ints = ints;
        });
    }

} // namespace hypha