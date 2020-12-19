#include <member.hpp>
#include <common.hpp>
#include <document_graph/document_graph.hpp>
#include <document_graph/document.hpp>
#include <document_graph/content.hpp>
#include <document_graph/edge.hpp>
#include <document_graph/util.hpp>
#include <util.hpp>

namespace hypha
{
    Member::Member(const eosio::name contract, const eosio::name &creator, const eosio::name &member)
        : Document(contract, contract, defaultContent(member))
    {
    }

    Member::Member(const eosio::name contract, const eosio::checksum256 &hash)
        : Document(contract, hash)
    {
    }

    Member Member::get(const eosio::name &contract, const eosio::name &member)
    {
        return Member(contract, Member::calcHash(member));
    }

    ContentGroups Member::defaultContent(const eosio::name &member)
    {
        return ContentGroups{
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, DETAILS),
                Content(MEMBER_STRING, member)},
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, SYSTEM),
                Content(TYPE, common::MEMBER),
                Content(NODE_LABEL, member.to_string())}};
    }

    const eosio::checksum256 Member::calcHash(const eosio::name &member)
    {
        auto cgs = Member::defaultContent(member);
        return Document::hashContents(cgs);
    }

    const bool Member::isMember(const eosio::name &rootNode, const eosio::name &member)
    {
        // create hash to represent this member account
        auto memberHash = Member::calcHash(member);
        return Edge::exists(rootNode, getRoot(rootNode), memberHash, common::MEMBER);
    }

    // Member Member::getOrNew(eosio::name contract, const eosio::name &creator, const eosio::name &member)
    // {
    //     return (Member) Document::getOrNew(contract, creator, Document::rollup(Content(MEMBER_STRING, member)));
    // }

    void Member::apply(const eosio::checksum256 &applyTo, const std::string content)
    {
        Edge::write(getContract(), getAccount(), applyTo, getHash(), common::APPLICANT);
        Edge::write(getContract(), getAccount(), getHash(), applyTo, common::APPLICANT_OF);
    }

    void Member::enroll(const eosio::name &enroller, const std::string &content)
    {
        // TODO: make this multi-member, it may not be "root"
        eosio::checksum256 root = getRoot(getContract());

        // create the new member edges
        Edge::write(getContract(), enroller, root, getHash(), common::MEMBER);
        Edge::write(getContract(), enroller, getHash(), root, common::MEMBER_OF);

        // remove the old applicant edges
        Edge rootApplicantEdge = Edge::get(getContract(), root, getHash(), common::APPLICANT);
        rootApplicantEdge.erase();

        Edge applicantRootEdge = Edge::get(getContract(), getHash(), root, common::APPLICANT_OF);
        applicantRootEdge.erase();
    }

    eosio::name Member::getAccount()
    {
        return getContentWrapper().getOrFail(DETAILS, MEMBER_STRING)->getAs<eosio::name>();
    }

} // namespace hypha