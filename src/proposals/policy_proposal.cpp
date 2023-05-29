#include <proposals/policy_proposal.hpp>
#include <common.hpp>
#include <util.hpp>

#include <document_graph/edge.hpp>

namespace hypha
{
    
void PolicyProposal::proposeImpl(const name &proposer, ContentWrapper &contentWrapper)
{}

void PolicyProposal::passImpl(Document &proposal)
{
    //Mark relationship from parent/grand parent/etc. to this doc
    markRelatives(common::MASTER_POLICY, name(), common::DESCENDANT, proposal.getID());
}

void PolicyProposal::failImpl(Document &proposal)
{
    //markRelatives(common::MASTER_POLICY, common::ASCENDANT, common::DESCENDANT, proposal.getID(), true);
}

void PolicyProposal::postProposeImpl(Document &proposal)
{
    auto cw = proposal.getContentWrapper();
    //Check for master policy
    if (auto masterPolicy = getItemDocOpt(common::MASTER_POLICY_ITEM, common::POLICY, cw)) {

        auto parentCW = masterPolicy->getContentWrapper();

        //Only approved quest can be used as parent quest
        EOS_CHECK(
            parentCW.getOrFail(DETAILS, common::STATE)
                    ->getAs<string>() == common::STATE_APPROVED,
            "Only approved Policy can be used as master policy"
        );

        //Create master policy edge
        Edge(m_dao.get_self(), m_dao.get_self(), proposal.getID(), masterPolicy->getID(), common::MASTER_POLICY);

        //Don't mark relationship from parent to child for now
        markRelatives(common::MASTER_POLICY, common::ASCENDANT, name(), proposal.getID());
    }

    //Check for parent circle
    if (auto parentCircle = getItemDocOpt(common::PARENT_CIRCLE_ITEM, common::CIRCLE, cw)) {
        //Make relationship to parent circle
        Edge(m_dao.get_self(), m_dao.get_self(), proposal.getID(), parentCircle->getID(), common::PARENT_CIRCLE);
    }
}

name PolicyProposal::getProposalType()
{
    return common::POLICY;
}

}