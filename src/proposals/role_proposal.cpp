#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>

#include <common.hpp>
#include <proposals/role_proposal.hpp>
#include <document_graph/content_group.hpp>

namespace hypha
{

    ContentGroups RoleProposal::propose_impl(const name &proposer, ContentGroups &content_groups)
    {
        // capacity is no longer enforced; commenting check
        // int64_t capacity = std::get<int64_t>(m_dao._document_graph.get_content(details, common::FULL_TIME_CAPACITY, true));
        // check(capacity > 0, "fulltime_capacity_x100 must be greater than zero. You submitted: " + std::to_string(capacity));

        eosio::asset annual_usd_salary = ContentWrapper::getContent (content_groups, common::DETAILS, common::ANNUAL_USD_SALARY).getAs<eosio::asset>();
        eosio::check (annual_usd_salary.amount > 0, common::ANNUAL_USD_SALARY + " must be greater than zero. You submitted: " + annual_usd_salary.to_string());
        eosio::print ("RoleProposal::propose_impl - salary: " + annual_usd_salary.to_string() + "\n");

        return content_groups;
    }

    Document RoleProposal::pass_impl(Document proposal)
    {
        ContentGroups cgs = Document::rollup(Content(common::ROOT_NODE, m_dao.get_self()));
        eosio::checksum256 rootNode = Document::hashContents(cgs);
        
        Edge rootRoleEdge (m_dao.get_self(), m_dao.get_self(), rootNode, proposal.getHash(), common::ROLE_NAME);
        rootRoleEdge.emplace();

        return proposal;
    }

    std::string RoleProposal::GetBallotContent (ContentGroups contentGroups)
    {
        return ContentWrapper::getContent (contentGroups, common::DETAILS, common::TITLE).getAs<std::string>();
    }
    
    name RoleProposal::GetProposalType () 
    {
        return common::ROLE_NAME;
    }

} // namespace hypha