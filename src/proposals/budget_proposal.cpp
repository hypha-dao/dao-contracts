#include <proposals/budget_proposal.hpp>

#include <common.hpp>
#include <typed_document.hpp>
#include <proposals/payout_proposal.hpp>

namespace hypha {

void BudgetProposal::proposeImpl(const name &proposer, ContentWrapper &contentWrapper)
{
    //Check token amounts
    PayoutProposal::checkTokenItems(m_daoSettings, contentWrapper);
    
    //Check if proposer is member of the circle
    auto circle = getItemDoc(common::CIRCLE_ID, common::CIRCLE, contentWrapper);

    auto proposerID = m_dao.getMemberID(proposer);

    //TODO: Enable member check
    //Edge::get(m_dao.get_self(), circle.getID(), proposerID, common::MEMBER);
}

void BudgetProposal::postProposeImpl(Document &proposal)
{
    auto cw = proposal.getContentWrapper();

    auto circle = getItemDoc(common::CIRCLE_ID, common::CIRCLE, cw);

    //Check circle doesn't have a budget already
    auto [hasBudget, budgetEdge] = Edge::getIfExists(m_dao.get_self(), circle.getID(), common::BUDGET);
    
    EOS_CHECK(
        !hasBudget,
        to_str("Circle has budget already: ", budgetEdge.getToNode())
    );

    auto [isLocked, lockedEdge] = Edge::getIfExists(m_dao.get_self(), circle.getID(), common::LOCKED_BY);

    EOS_CHECK(
        !isLocked,
        to_str("Circle has an open budget proposal already: ", lockedEdge.getToNode())
    );

    //Create circle link
    Edge(m_dao.get_self(), m_dao.get_self(), proposal.getID(), circle.getID(), common::CIRCLE);

    //Lock the circle (will be automatically unlocked if the proposal fails)
    Edge(m_dao.get_self(), m_dao.get_self(), circle.getID(), proposal.getID(), common::LOCKED_BY);
}

void BudgetProposal::passImpl(Document &proposal)
{
    auto cw = proposal.getContentWrapper();
    
    auto circle = getLinkDoc(proposal.getID(), common::CIRCLE, common::CIRCLE);

    //Make budget link
    Edge(m_dao.get_self(), m_dao.get_self(), circle.getID(), proposal.getID(), common::BUDGET);
}

void BudgetProposal::failImpl(Document &proposal)
{   
    auto circle = getLinkDoc(proposal.getID(), common::CIRCLE, common::CIRCLE);

    //Stop locking the Circle
    Edge::getTo(m_dao.get_self(), proposal.getID(), common::LOCKED_BY).erase();
}

string BudgetProposal::getBallotContent(ContentWrapper &contentWrapper)
{
    return getTitle(contentWrapper);
}

name BudgetProposal::getProposalType()
{
    return common::BUDGET;
}


}