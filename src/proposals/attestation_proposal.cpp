#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>

#include <common.hpp>
#include <util.hpp>
#include <proposals/attestation_proposal.hpp>
#include <document_graph/content_wrapper.hpp>
#include <logger/logger.hpp>

namespace hypha
{

    void AttestationProposal::proposeImpl(const name &proposer, ContentWrapper &contentWrapper)
    { }

    void AttestationProposal::passImpl(Document &proposal)
    {
        TRACE_FUNCTION()
        Edge::write (m_dao.get_self(), m_dao.get_self(), getRoot(m_dao.get_self()), proposal.getHash(), common::ATTESTATION);
    }

    std::string AttestationProposal::getBallotContent (ContentWrapper &contentWrapper)
    {
        TRACE_FUNCTION()
        return contentWrapper.getOrFail(DETAILS, TITLE)->getAs<std::string>();
    }
    
    name AttestationProposal::getProposalType () 
    {
        return common::ATTESTATION;
    }

} // namespace hypha