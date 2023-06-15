#include <proposals/circle_proposal.hpp>

namespace hypha {

void CircleProposal::proposeImpl(const name &proposer, ContentWrapper &contentWrapper) 
{
    //Check for circle name and purpose. Note: might need to add some character
    //limits
    //Also we might want to check for duplicated circle names
    auto name = contentWrapper.getOrFail(DETAILS, common::CIRCLE_NAME)->getAs<std::string>();

    EOS_CHECK(
        name.size() <= 64,
        "Circle name is too large, max characters is 64"
    )

    auto purpose = contentWrapper.getOrFail(DETAILS, common::CIRCLE_PURPOSE)->getAs<std::string>();

    EOS_CHECK(
        purpose.size() < 512,
        "Circle purpose is too large, max characters is 512"
    )

    if (auto [_, isAutoApprove] = contentWrapper.get(DETAILS, common::AUTO_APPROVE); 
        isAutoApprove && isAutoApprove->getAs<int64_t>() == 1) {
        //Only admins of the DAO are allowed to create this type of proposals
        m_dao.checkAdminsAuth(m_daoID);
        selfApprove = true;
    }
}

void CircleProposal::postProposeImpl(Document &proposal) 
{
    auto cw = proposal.getContentWrapper();

    if (auto parentCircle = getItemDocOpt(common::PARENT_CIRCLE_ITEM, common::CIRCLE, cw)) {
        //Make relationship to parent
        Edge(m_dao.get_self(), m_dao.get_self(), proposal.getID(), parentCircle->getID(), common::PARENT_CIRCLE);
    }
}

void CircleProposal::passImpl(Document &proposal) 
{
    auto cw = proposal.getContentWrapper();

    if (auto parentCircle = getItemDocOpt(common::PARENT_CIRCLE_ITEM, common::CIRCLE, cw)) {
        //Make relationship from parent
        Edge(m_dao.get_self(), m_dao.get_self(), parentCircle->getID(), proposal.getID(), common::SUB_CIRCLE);
    }
    else {
        //Make circle top level
        Edge(m_dao.get_self(), m_dao.get_self(), m_daoID, proposal.getID(), common::CIRCLE);
    }
}

string CircleProposal::getBallotContent(ContentWrapper &contentWrapper)
{
    return contentWrapper.getOrFail(DETAILS, TITLE)->getAs<std::string>();
}

name CircleProposal::getProposalType() 
{
    return common::CIRCLE;
}

}