#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>

#include <common.hpp>
#include <util.hpp>
#include <proposals/role_proposal.hpp>
#include <document_graph/content_wrapper.hpp>
#include <logger/logger.hpp>

namespace hypha
{

    void RoleProposal::proposeImpl(const name &proposer, ContentWrapper &contentWrapper)
    {
        TRACE_FUNCTION()
        // capacity is no longer enforced; commenting check
        // int64_t capacity = std::get<int64_t>(m_dao._document_graph.get_content(details, common::FULL_TIME_CAPACITY, true));
        // check(capacity > 0, "fulltime_capacity_x100 must be greater than zero. You submitted: " + std::to_string(capacity));

        eosio::asset annual_usd_salary = contentWrapper.getOrFail(DETAILS, ANNUAL_USD_SALARY)->getAs<eosio::asset>();
        eosio::check (annual_usd_salary.amount > 0, ANNUAL_USD_SALARY + string(" must be greater than zero. You submitted: ") + annual_usd_salary.to_string());

    }

    void RoleProposal::passImpl(Document &proposal)
    {
        TRACE_FUNCTION()
        // eosio::checksum256 rootNode = ;
        Edge::write (m_dao.get_self(), m_dao.get_self(), getRoot(m_dao.get_self()), proposal.getHash(), common::ROLE_NAME);
    }

    std::string RoleProposal::getBallotContent (ContentWrapper &contentWrapper)
    {
        TRACE_FUNCTION()
        return contentWrapper.getOrFail(DETAILS, TITLE)->getAs<std::string>();
    }
    
    name RoleProposal::getProposalType () 
    {
        return common::ROLE_NAME;
    }

} // namespace hypha