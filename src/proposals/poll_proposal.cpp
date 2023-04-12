#include <proposals/poll_proposal.hpp>
#include <common.hpp>
#include <util.hpp>

namespace hypha
{

void PollProposal::postProposeImpl(Document &proposal)
{
    auto cw = proposal.getContentWrapper();

    auto votingType = cw.getOrFail(DETAILS, "voting_method")->getAs<std::string>();

    //Check if proposal is of type community or core, so we can use one type of vote system or the other
    if (votingType == "Community") {
        ContentWrapper::insertOrReplace(
            *cw.getGroupOrFail(SYSTEM),
            Content{ "community_voting", 1 }
        );
    }
}

name PollProposal::getProposalType()
{
    return common::POLL;
}

}