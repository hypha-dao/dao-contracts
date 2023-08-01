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

        if (auto [_, isAutoApprove] = contentWrapper.get(DETAILS, common::AUTO_APPROVE); 
            isAutoApprove && isAutoApprove->getAs<int64_t>() == 1) {
            //Only admins of the DAO are allowed to create this type of proposals
            m_dao.checkAdminsAuth(m_daoID);
            selfApprove = true;
        }
    }

    void RoleProposal::passImpl(Document &proposal)
    {
        TRACE_FUNCTION()
        Edge::write (m_dao.get_self(), m_dao.get_self(), m_daoID, proposal.getID(), common::ROLE_NAME);
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