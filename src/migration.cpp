#include <common.hpp>
#include <document_graph/document_graph.hpp>
#include <document_graph/document.hpp>
#include <document_graph/content.hpp>
#include <document_graph/edge.hpp>
#include <document_graph/util.hpp>
#include <payers/payer.hpp>
#include <assignment.hpp>
#include <migration.hpp>
#include <member.hpp>
#include <util.hpp>
#include <dao.hpp>

namespace hypha
{

    Migration::Migration(dao *dao) : m_dao{dao} {}

    void Migration::migrateRole(const uint64_t &roleId)
    {
        eosio::name scope = eosio::name("role");

        Migration::object_table o_t_role(m_dao->get_self(), scope.value);
        auto o_itr_role = o_t_role.find(roleId);
        eosio::check(o_itr_role != o_t_role.end(), "roleId does not exist: " + std::to_string(roleId));

        // create the new document
        ContentGroups role = newContentGroups(o_itr_role->id,
                                               scope,
                                               o_itr_role->created_date,
                                               o_itr_role->names,
                                               o_itr_role->strings,
                                               o_itr_role->assets,
                                               o_itr_role->time_points,
                                               o_itr_role->ints);

        Document roleDocument (m_dao->get_self(), m_dao->get_self(), role);

        //  member          ---- owns       ---->   roleDocument
        //  roleDocument    ---- ownedBy    ---->   member
        // ContentWrapper cw(role);
        // auto systemsGroup = cw.getGroupOrFail(SYSTEM);
        // ContentWrapper::insertOrReplace(*systemsGroup,
        //                                 Content{NODE_LABEL,
        //                                         startPeriod.getNodeLabel()});

        // cw.removeContent(DETAILS, "assigned_account");
        // cw.removeContent(DETAILS, "fk");
        // cw.removeContent(DETAILS, "role_id");

        auto owner = hypha::Member::get(m_dao->get_self(), o_itr_role->names.at("owner"));

        Edge::write(m_dao->get_self(), m_dao->get_self(), owner.getHash(), roleDocument.getHash(), common::OWNS);
        Edge::write(m_dao->get_self(), m_dao->get_self(), roleDocument.getHash(), owner.getHash(), common::OWNED_BY);
        Edge::write(m_dao->get_self(), m_dao->get_self(), getRoot(m_dao->get_self()), roleDocument.getHash(), common::ROLE_NAME);

        newXRef(roleDocument.getHash(), scope, roleId);
        o_t_role.erase(o_itr_role);
    }

    void Migration::newXRef(eosio::checksum256 hash, eosio::name scope, uint64_t id)
    {
        // add the cross-reference to our temporary migration lookup table
        XReferenceTable xrt(m_dao->get_self(), scope.value);
        xrt.emplace(m_dao->get_self(), [&](auto &x) {
            x.id = id;
            x.hash = hash;
        });
    }

    eosio::checksum256 Migration::getXRef(eosio::name scope, uint64_t id)
    {
        XReferenceTable xrt(m_dao->get_self(), scope.value);
        auto xrt_itr = xrt.find(id);
        eosio::check(xrt_itr != xrt.end(), "object not found in xref: " + scope.to_string() + ", ID: " + std::to_string(id));

        return xrt_itr->hash;
    }

    ContentGroups Migration::newMemo(const std::string memo)
    {
        return ContentGroups{
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, DETAILS),
                Content(MEMO, memo)},
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, SYSTEM),
                Content(TYPE, eosio::name("memo")),
                Content(NODE_LABEL, "Migration Memo")}};
    }

    void Migration::migrateAssignment(const uint64_t &assignmentId)
    {
        eosio::name scope = eosio::name("assignment");

        Migration::object_table o_t(m_dao->get_self(), scope.value);
        auto assignment_itr = o_t.find(assignmentId);
        eosio::check(assignment_itr != o_t.end(), "assignmentId does not exist: " + std::to_string(assignmentId));

        // convert roleId to role hash
        auto roleHash = getXRef(eosio::name("role"), assignment_itr->ints.at("role_id"));

        // convert start_period to start_period hash
        auto startPeriodHash = getXRef(common::PERIOD, assignment_itr->ints.at(START_PERIOD));
        int64_t periodCount = assignment_itr->ints.at(END_PERIOD) - assignment_itr->ints.at(START_PERIOD);

        Period startPeriod(m_dao, startPeriodHash);

        // create the new document
        ContentGroups assignment = newContentGroups(assignment_itr->id,
                                                    scope,
                                                    assignment_itr->created_date,
                                                    assignment_itr->names,
                                                    assignment_itr->strings,
                                                    assignment_itr->assets,
                                                    assignment_itr->time_points,
                                                    assignment_itr->ints);

        ContentWrapper cw(assignment);
        auto detailsGroup = cw.getGroupOrFail(DETAILS);
        ContentWrapper::insertOrReplace(*detailsGroup, Content{START_PERIOD, startPeriod.getHash()});
        ContentWrapper::insertOrReplace(*detailsGroup, Content{PERIOD_COUNT, periodCount});
        ContentWrapper::insertOrReplace(*detailsGroup, Content{ROLE_STRING, roleHash});
        ContentWrapper::insertOrReplace(*detailsGroup, Content{ASSIGNEE, assignment_itr->names.at("assigned_account")});

        auto systemsGroup = cw.getGroupOrFail(SYSTEM);
        ContentWrapper::insertOrReplace(*systemsGroup,
                                        Content{NODE_LABEL,
                                                assignment_itr->names.at("assigned_account").to_string() + ": " +
                                                    startPeriod.getNodeLabel()});

        cw.removeContent(DETAILS, "assigned_account");
        cw.removeContent(DETAILS, "fk");
        cw.removeContent(DETAILS, "role_id");

        Assignment assmnt(m_dao, cw);
        eosio::checksum256 assigneeHash = hypha::Member::calcHash(assignment_itr->names.at("assigned_account"));

        if (!Document::exists(m_dao->get_self(), assigneeHash))
        {
            Document assigneeDoc = Document::getOrNew(m_dao->get_self(), m_dao->get_self(), newMemo("account was not a member on migration"));
            assigneeHash = assigneeDoc.getHash();
        }

        Edge::write(m_dao->get_self(), m_dao->get_self(), assigneeHash, assmnt.getHash(), common::ASSIGNED);
        Edge::write(m_dao->get_self(), m_dao->get_self(), assmnt.getHash(), assigneeHash, common::ASSIGNEE_NAME);
        Edge::write(m_dao->get_self(), m_dao->get_self(), roleHash, assmnt.getHash(), common::ASSIGNMENT);
        Edge::write(m_dao->get_self(), m_dao->get_self(), assmnt.getHash(), startPeriod.getHash(), common::START);

        newXRef(assmnt.getHash(), scope, assignmentId);

        // erase the table record
        o_t.erase(assignment_itr);
    }

    void Migration::migrateAssPayout (const uint64_t &ass_payout_id)
    {
        // get asspayout        
        asspay_table asspay_t(m_dao->get_self(), m_dao->get_self().value);
        auto asspay_itr = asspay_t.find (ass_payout_id);
        eosio::check (asspay_itr != asspay_t.end(), "ass_pay_id not found");

        // find the assignment
        eosio::checksum256 assignmentHash = getXRef(eosio::name("assignment"), asspay_itr->assignment_id);

        // find the recipient / member
        eosio::checksum256 recipientHash = hypha::Member::calcHash(asspay_itr->recipient);
        if (!Document::exists(m_dao->get_self(), recipientHash))
        {
            Document recipientDoc = Document::getOrNew(m_dao->get_self(), m_dao->get_self(), newMemo("account was not a member on migration"));
            recipientHash = recipientDoc.getHash();
        }

        // find the period
        eosio::checksum256 periodHash = getXRef(eosio::name("period"), asspay_itr->period_id);
        Edge::write(m_dao->get_self(), m_dao->get_self(), assignmentHash, periodHash, common::CLAIMED);

        bool writtenXRef = false;
        for (auto& payment : asspay_itr->payments) {

            if (payment.amount > 0) {

                // create receipt for each payment
                ContentGroups receipt = Payer::defaultReceipt(asspay_itr->recipient, payment, "created during migration");
                ContentWrapper cw (receipt);
                auto detailsGroup = cw.getGroupOrFail(DETAILS);
                ContentWrapper::insertOrReplace(*detailsGroup, Content{"payment_date", asspay_itr->payment_date});

                // TODO: need to review if this is the best way to do this (replicates the data)
                // putting the periodHash inside the receipt creates document uniqueness for each period's payment
                ContentWrapper::insertOrReplace(*detailsGroup, Content{"period_hash", periodHash});
                ContentWrapper::insertOrReplace(*detailsGroup, Content{"recipient_hash", recipientHash});
                ContentWrapper::insertOrReplace(*detailsGroup, Content{"assignment_hash", assignmentHash});

                Document paymentReceipt = Document::getOrNew(m_dao->get_self(), m_dao->get_self(), receipt);

                // 1 legacy asspayout record becomes many documents, but we'll only tag the first receipt in the xref table
                if (!writtenXRef) {
                    // newXRef(paymentReceipt.getHash(), eosio::name("asspay"), asspay_itr->ass_payment_id);
                    writtenXRef = true;
                }

                Edge::write(m_dao->get_self(), m_dao->get_self(), periodHash, paymentReceipt.getHash(), common::PAYMENT);
                Edge::write(m_dao->get_self(), m_dao->get_self(), recipientHash, paymentReceipt.getHash(), common::PAID);
            }
        }
        asspay_t.erase (asspay_itr);
    }

    void Migration::addApplicant(const eosio::name &applicant, const std::string content)
    {
        Migration::applicant_table a_t(m_dao->get_self(), m_dao->get_self().value);
        if (a_t.find(applicant.value) != a_t.end())
        {
            return;
        }

        a_t.emplace(m_dao->get_self(), [&](auto &a) {
            a.applicant = applicant;
            a.content = content;
        });
    }

    int Migration::defSetSetting(const string &key, const Content::FlexValue &value, int senderId)
    {
        eosio::transaction out{};
        out.actions.emplace_back(
            eosio::permission_level{m_dao->get_self(), name("active")},
            m_dao->get_self(), name("setsetting"),
            make_tuple(key, value));

        out.delay_sec = 1;
        out.send(senderId, m_dao->get_self());
        return senderId;
    }

    void Migration::migrateConfig()
    {
        config_table config_s(m_dao->get_self(), m_dao->get_self().value);
        Config c = config_s.get_or_create(m_dao->get_self(), Config());
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

    void Migration::addMemberToTable(const eosio::name &member)
    {
        Migration::member_table m_t(m_dao->get_self(), m_dao->get_self().value);
        if (m_t.find(member.value) != m_t.end())
        {
            return;
        }

        m_t.emplace(m_dao->get_self(), [&](auto &m) {
            m.member = member;
        });
    }
    void Migration::migrateMember(const eosio::name &memberName)
    {
        eosio::require_auth(m_dao->get_self());

        // create the member document
        hypha::Member member(m_dao->get_self(), m_dao->get_self(), memberName);
        eosio::checksum256 root = getRoot(m_dao->get_self());

        // create the new member edges
        Edge::write(m_dao->get_self(), m_dao->get_self(), root, member.getHash(), common::MEMBER);
        Edge::write(m_dao->get_self(), m_dao->get_self(), member.getHash(), root, common::MEMBER_OF);

        Migration::member_table m_t(m_dao->get_self(), m_dao->get_self().value);
        auto m_itr = m_t.find(memberName.value);
        eosio::check(m_itr != m_t.end(), "member not in members table: " + memberName.to_string());
        m_t.erase(m_itr);
    }

    void Migration::addPeriodToTable(const uint64_t &id,
                                     const time_point &start_date,
                                     const time_point &end_date,
                                     const string &phase,
                                     const string &readable,
                                     const string &label)
    {
        Migration::period_table period_t(m_dao->get_self(), m_dao->get_self().value);
        period_t.emplace(m_dao->get_self(), [&](auto &p) {
            p.period_id = period_t.available_primary_key();
            p.start_date = start_date;
            p.end_date = end_date;
            p.phase = phase;
            p.label = label;
            p.readable = readable;
        });
    }
    void Migration::migratePeriod(const uint64_t &id)
    {
        eosio::require_auth(m_dao->get_self());

        Migration::period_table period_t(m_dao->get_self(), m_dao->get_self().value);
        auto p_itr = period_t.find(id);

        Period newPeriod(m_dao, p_itr->start_date, p_itr->phase, p_itr->readable, p_itr->label);
        newXRef(newPeriod.getHash(), eosio::name("period"), id);

        eosio::checksum256 predecessor;
        if (id == 0)
        {
            predecessor = getRoot(m_dao->get_self());
        }
        else
        {
            predecessor = getXRef(eosio::name("period"), id - 1);
        }

        // check that predecessor exists
        Document previous(m_dao->get_self(), predecessor);
        ContentWrapper contentWrapper = previous.getContentWrapper();

        // if it's a period, make sure the start time comes before
        auto [idx, predecessorType] = contentWrapper.get(SYSTEM, TYPE);
        if (predecessorType && predecessorType->getAs<eosio::name>() == common::PERIOD)
        {
            eosio::check(contentWrapper.getOrFail(DETAILS, START_TIME)->getAs<eosio::time_point>().sec_since_epoch() <
                             p_itr->start_date.sec_since_epoch(),
                         "start_time of period predecessor must be before the new period's start_time");

            // predecessor is a period, so use "next" edge
            Edge::write(m_dao->get_self(), m_dao->get_self(), predecessor, newPeriod.getHash(), common::NEXT);
        }
        else
        {
            // predecessor is not a period, so use "start" edge
            Edge::write(m_dao->get_self(), m_dao->get_self(), predecessor, newPeriod.getHash(), common::START);
        }

        period_t.erase(p_itr);
    }

    void Migration::addAssPayoutToTable(const uint64_t &ass_payment_id,
                                        const uint64_t &assignment_id,
                                        const name &recipient,
                                        uint64_t period_id,
                                        std::vector<eosio::asset> payments,
                                        time_point payment_date)
    {
        asspay_table asspay_t(m_dao->get_self(), m_dao->get_self().value);
        asspay_t.emplace(m_dao->get_self(), [&](auto &a) {
            a.ass_payment_id = ass_payment_id;
            a.assignment_id = assignment_id;
            a.recipient = recipient,
            a.period_id = period_id;
            a.payment_date = payment_date;
            a.payments = payments;
        });      
    }
    void Migration::eraseAllObjects(const eosio::name &scope)
    {
        Migration::object_table o_t(m_dao->get_self(), scope.value);
        auto o_itr = o_t.begin();
        while (o_itr != o_t.end())
        {
            o_itr = o_t.erase(o_itr);
        }
    }

    void Migration::eraseXRefs(const eosio::name &scope)
    {
        XReferenceTable xrt_t(m_dao->get_self(), scope.value);
        auto x_itr = xrt_t.begin();
        while (x_itr != xrt_t.end())
        {
            x_itr = xrt_t.erase(x_itr);
        }
    }

    void Migration::eraseAssPayouts()
    {
        eosio::require_auth(m_dao->get_self());

        Migration::asspay_table period_t(m_dao->get_self(), m_dao->get_self().value);
        auto p_itr = period_t.begin();
        while (p_itr != period_t.end())
        {
            p_itr = period_t.erase(p_itr);
        }
    }

    void Migration::erasePeriods()
    {
        eosio::require_auth(m_dao->get_self());

        Migration::period_table period_t(m_dao->get_self(), m_dao->get_self().value);
        auto p_itr = period_t.begin();
        while (p_itr != period_t.end())
        {
            p_itr = period_t.erase(p_itr);
        }
    }

    void Migration::eraseGraph()
    {
        eosio::require_auth(m_dao->get_self());

        Edge::edge_table e_t(m_dao->get_self(), m_dao->get_self().value);
        auto e_itr = e_t.begin();
        while (e_itr != e_t.end())
        {
            e_itr = e_t.erase(e_itr);
        }

        Document::document_table d_t(m_dao->get_self(), m_dao->get_self().value);
        auto d_itr = d_t.begin();
        while (d_itr != d_t.end())
        {
            d_itr = d_t.erase(d_itr);
        }
    }

    void Migration::reset4test()
    {
        eosio::require_auth(m_dao->get_self());

        Document settingsDocument = m_dao->getSettingsDocument();
        ContentWrapper cw = settingsDocument.getContentWrapper();
        ContentGroup *settings = cw.getGroupOrFail("settings");

        eraseXRefs(eosio::name("role"));
        eraseXRefs(eosio::name("assignment"));
        eraseXRefs(eosio::name("period"));
        eraseXRefs(eosio::name("asspay"));

        eraseGraph();
        erasePeriods();
        eraseAssPayouts();

        ContentGroups newSettingsCGS{
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, SYSTEM),
                Content(TYPE, common::SETTINGS_EDGE),
                Content(NODE_LABEL, "Settings")},
            *settings};

        Document updatedSettings(m_dao->get_self(), m_dao->get_self(), newSettingsCGS);
        Document rootDocument(m_dao->get_self(), m_dao->get_self(), getRootContent(m_dao->get_self()));
        Edge::write(m_dao->get_self(), m_dao->get_self(), rootDocument.getHash(), updatedSettings.getHash(), common::SETTINGS_EDGE);

        m_dao->setSetting(ROOT_NODE, rootDocument.getHash());
    }

    ContentGroups Migration::newContentGroups(const uint64_t id,
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
            if (string_itr->second != "" && string_itr->second != "null")
            {
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
        }

        std::map<string, uint64_t>::const_iterator int_itr;
        for (int_itr = ints.begin(); int_itr != ints.end(); ++int_itr)
        {
            Content content{int_itr->first, int_itr->second};
            if (int_itr->first == "object_id" ||
                int_itr->first == "prior_id" ||
                int_itr->first == "original_object_id" ||
                int_itr->first == "revisions")
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

        return contentGroups;
    }

    void Migration::newObject(const uint64_t &id,
                              const name &scope,
                              map<string, name> names,
                              map<string, string> strings,
                              map<string, asset> assets,
                              map<string, time_point> time_points,
                              map<string, uint64_t> ints)
    {
        Migration::object_table o_t(m_dao->get_self(), scope.value);
        o_t.emplace(m_dao->get_self(), [&](auto &o) {
            o.id = id;
            o.names = names;
            o.strings = strings;
            o.assets = assets;
            o.time_points = time_points;
            o.ints = ints;
        });
    }

} // namespace hypha