#include <proposals/circle_proposal.hpp>
#include <common.hpp>
#include <string>
#include <circles/circle.hpp>

using std::string;

namespace hypha
{
    const string ENTRY_AUTHOR = "author";
    const string ENTRY_TITLE = "title";
    const string ENTRY_DESCRIPTION = "description";
    const string ENTRY_BUDGET = "budget";
    const string ENTRY_PARENT_CIRCLE = "parent_circle";

    void CircleProposal::proposeImpl(const name &proposer, ContentWrapper &contentWrapper)
    {
        TRACE_FUNCTION()

        contentWrapper.getOrFail(DETAILS, ENTRY_BUDGET)->getAs<asset>();

        auto parentCircleId = contentWrapper.getOrFail(DETAILS, ENTRY_PARENT_CIRCLE)->getAs<int64_t>();
        if (parentCircleId != 0) {
            Circle(m_dao, parentCircleId);
        }

        contentWrapper.getOrFail(DETAILS, ENTRY_PARENT_CIRCLE)->getAs<int64_t>();
        contentWrapper.getOrFail(DETAILS, ENTRY_TITLE)->getAs<string>();
        contentWrapper.getOrFail(DETAILS, ENTRY_DESCRIPTION)->getAs<string>();
        contentWrapper.getOrFail(DETAILS, ENTRY_BUDGET)->getAs<asset>();

        ContentWrapper::insertOrReplace(
            *contentWrapper.getGroupOrFail(DETAILS),
            Content { ENTRY_AUTHOR, proposer }
        );
    }

    void CircleProposal::postProposeImpl (Document &proposal)
    {
    }

    void CircleProposal::passImpl(Document &proposal)
    {
        TRACE_FUNCTION()
        auto contentWrapper = proposal.getContentWrapper();

        auto parentCircleId = contentWrapper.getOrFail(DETAILS, ENTRY_PARENT_CIRCLE)->getAs<int64_t>();
        auto author = contentWrapper.getOrFail(DETAILS, ENTRY_AUTHOR)->getAs<name>();
        auto title = contentWrapper.getOrFail(DETAILS, ENTRY_TITLE)->getAs<string>();
        auto description = contentWrapper.getOrFail(DETAILS, ENTRY_DESCRIPTION)->getAs<string>();
        auto budget = contentWrapper.getOrFail(DETAILS, ENTRY_BUDGET)->getAs<asset>();

        Circle(
            m_dao,
            author,
            title,
            description,
            m_daoID,
            parentCircleId,
            budget
        );
    }

    string CircleProposal::getBallotContent(ContentWrapper &contentWrapper)
    {
        return string{"Circle proposal"};
    }

    name CircleProposal::getProposalType()
    {
        return common::CIRCLE;
    }
}
