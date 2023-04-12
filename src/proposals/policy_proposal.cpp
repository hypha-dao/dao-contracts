#include <proposals/policy_proposal.hpp>
#include <common.hpp>
#include <util.hpp>

#include <document_graph/edge.hpp>

namespace hypha
{

bool PolicyProposal::checkMembership(const eosio::name& proposer, ContentGroups &contentGroups)
{
    //We only allow core members to create policy proposals
    return false;
}

void PolicyProposal::proposeImpl(const name &proposer, ContentWrapper &contentWrapper)
{}

void PolicyProposal::passImpl(Document &proposal)
{}

void PolicyProposal::failImpl(Document &proposal)
{
    markRelatives(common::MASTER_POLICY, common::ASCENDANT, common::DESCENDANT, proposal.getID(), true);
}

void PolicyProposal::postProposeImpl(Document &proposal)
{
    auto cw = proposal.getContentWrapper();
    //Check for master policy
    if (auto masterPolicy = getParent(common::MASTER_POLICY_ITEM, common::POLICY, cw)) {

        auto parentCW = masterPolicy->getContentWrapper();

        //Only approved quest can be used as parent quest
        EOS_CHECK(
            parentCW.getOrFail(DETAILS, common::STATE)
                    ->getAs<string>() == common::STATE_APPROVED,
            "Only approved Policy can be used as master policy"
        );

        //Create master policy edge
        Edge(m_dao.get_self(), m_dao.get_self(), proposal.getID(), masterPolicy->getID(), common::MASTER_POLICY);

        markRelatives(common::MASTER_POLICY, common::ASCENDANT, common::DESCENDANT, proposal.getID());
    }
}

name PolicyProposal::getProposalType()
{
    return common::POLICY;
}

}