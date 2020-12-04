#include <proposals/badge_proposal.hpp>
#include <document_graph/content_wrapper.hpp>
#include <util.hpp>

namespace hypha
{
    void BadgeProposal::propose_impl(const name &proposer, ContentWrapper &badge)
    {
        // check coefficients
        // TODO: move coeffecient thresholds to be configuration values
        checkCoefficient(badge, common::HUSD_COEFFICIENT);
        checkCoefficient(badge, common::HYPHA_COEFFICIENT);
        checkCoefficient(badge, common::HVOICE_COEFFICIENT);
        checkCoefficient(badge, common::SEEDS_COEFFICIENT);
    }

    void BadgeProposal::pass_impl(Document &proposal)
    {
        Edge::write(m_dao.get_self(), m_dao.get_self(), getRoot(m_dao.get_self()), proposal.getHash(), common::BADGE_NAME);
    }

    void BadgeProposal::checkCoefficient(ContentWrapper &badge, const std::string &key)
    {
        if (auto [idx, coefficient] = badge.get(common::DETAILS, key); coefficient)
        {
            eosio::check(std::holds_alternative<int64_t>(coefficient->value), "fatal error: coefficient must be an int64_t type: " + coefficient->label);
            eosio::check(std::get<int64_t>(coefficient->value) >= 7000 &&
                             std::get<int64_t>(coefficient->value) <= 13000,
                         "fatal error: coefficient_x10000 must be between 7000 and 13000, inclusive: " + coefficient->label);
        }
    }

    std::string BadgeProposal::GetBallotContent(ContentWrapper &contentWrapper)
    {
        return contentWrapper.getOrFail(common::DETAILS, common::ICON)->getAs<std::string>();
    }

    name BadgeProposal::GetProposalType()
    {
        return common::BADGE_NAME;
    }

} // namespace hypha