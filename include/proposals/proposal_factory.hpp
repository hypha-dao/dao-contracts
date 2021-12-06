#pragma once 

#include <eosio/name.hpp>
#include "proposal.hpp"

namespace hypha {

    class dao;
    // class Proposal;

    class ProposalFactory
    {
    public:
        static Proposal* Factory(dao &dao, const checksum256& daoHash, const eosio::name &proposal_type);
    };
}


