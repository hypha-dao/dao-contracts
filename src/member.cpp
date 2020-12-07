#include <member.hpp>
#include <common.hpp>
#include <document_graph/document_graph.hpp>
#include <document_graph/content.hpp>
#include <document_graph/edge.hpp>
#include <document_graph/util.hpp>
#include <util.hpp>

namespace hypha
{
    Member::Member(const eosio::name contract, const eosio::name &creator, const eosio::name &member)
        : m_contract{contract}, m_member{member}
    {
        m_document = Member::getOrNew(contract, creator, m_member);
    }

    const eosio::checksum256 Member::getHash(const eosio::name &member)
    {
        ContentGroups contentGroups = Document::rollup(Content(MEMBER_STRING, member));
        return Document::hashContents(contentGroups);
    }

    const bool Member::isMember(const eosio::name &rootNode, const eosio::name &member)
    {
        // create hash to represent this member account
        auto memberHash = Member::getHash(member);
        return Edge::exists(rootNode, getRoot(rootNode), memberHash, common::MEMBER);
    }

    Document Member::getOrNew(eosio::name contract, const eosio::name &creator, const eosio::name &member)
    {
        return Document::getOrNew(contract, creator, Document::rollup(Content(MEMBER_STRING, member)));
    }

    void Member::apply(const eosio::checksum256 &applyTo, const std::string content)
    {
        Edge::write(m_contract, m_member, applyTo, m_document.getHash(), common::APPLICANT);
        Edge::write(m_contract, m_member, m_document.getHash(), applyTo, common::APPLICANT_OF);
    }

    void Member::enroll(const eosio::name &enroller, const std::string &content)
    {
        // TODO: make this multi-member, it may not be "root"
        eosio::checksum256 root = getRoot(m_contract);

        // create the new member edges
        Edge::write(m_contract, enroller, root, m_document.getHash(), common::MEMBER);
        Edge::write(m_contract, enroller, m_document.getHash(), root, common::MEMBER_OF);

        // remove the old applicant edges
        Edge rootApplicantEdge = Edge::get(m_contract, root, m_document.getHash(), common::APPLICANT);
        rootApplicantEdge.erase();

        Edge applicantRootEdge = Edge::get(m_contract, m_document.getHash(), root, common::APPLICANT_OF);
        applicantRootEdge.erase();
    }

} // namespace hypha