#include <proposals/circle_suspend_proposal.hpp>
#include <common.hpp>
#include <string>
#include <circles/circle.hpp>

using std::string;

namespace hypha
{

    const string ENTRY_CIRCLE_ID = "circle_id";

    void CircleSuspendProposal::proposeImpl(const name &proposer, ContentWrapper &contentWrapper)
    {
        TRACE_FUNCTION()

        // Checks that the circle exists and is not suspended
        Circle circle(m_dao, contentWrapper.getOrFail(DETAILS, ENTRY_CIRCLE_ID)->getAs<int64_t>());
    }

    void CircleSuspendProposal::postProposeImpl(Document &proposal)
    {
    }

    void CircleSuspendProposal::passImpl(Document &proposal)
    {
        TRACE_FUNCTION()

        auto contentWrapper = proposal.getContentWrapper();
        Circle circle(m_dao, contentWrapper.getOrFail(DETAILS, ENTRY_CIRCLE_ID)->getAs<int64_t>());
        circle.remove();
    }

    string CircleSuspendProposal::getBallotContent(ContentWrapper &contentWrapper)
    {
        return "Circle suspend proposal";
    }

    name CircleSuspendProposal::getProposalType()
    {
        return common::CIRCLE_SUSPEND;
    }
}
