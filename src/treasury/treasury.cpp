#include "treasury/treasury.hpp"

#include <dao.hpp>
#include <common.hpp>
#include <settings.hpp>
#include <member.hpp>

#include "treasury/common.hpp"

#ifdef USE_TREASURY

namespace hypha::treasury {

using namespace common;

Treasury::Treasury(dao& dao, uint64_t id)
    : TypedDocument(dao, id, types::TREASURY)
{}

Treasury::Treasury(dao& dao, Data data)
    : TypedDocument(dao, types::TREASURY)
{   
    auto daoID = data.dao;

    EOS_CHECK(
        dao.getGraph().getEdgesFrom(daoID, links::TREASURY).empty(),
        "DAO can only have 1 treasury"
    )

    auto cgs = convert(std::move(data));

    initializeDocument(dao, cgs);

    auto settings = Settings(dao, getId());

    //Default settings
    settings.setSettings(SETTINGS, {
        { fields::THRESHOLD, 1 },
        { fields::REDEMPTIONS_ENABLED, 1 }
    });

    //Initialize edges to/from DAO
    Edge(
        dao.get_self(), 
        dao.get_self(), 
        daoID,
        getId(),
        links::TREASURY
    );

    Edge(
        dao.get_self(), 
        dao.get_self(), 
        getId(), 
        daoID, 
        links::TREASURY_OF
    );
}

void Treasury::removeTreasurer(uint64_t memberID)
{
    //Remove the edge (if it doesn't exist it will fail)
    Edge::get(getDao().get_self(), 
              getId(),
              memberID,
              links::TREASURER).erase();

    Edge::get(getDao().get_self(), 
              memberID,
              getId(),
              links::TREASURER_OF).erase();
}

void Treasury::addTreasurer(uint64_t memberID)
{
    Edge::getOrNew(getDao().get_self(), 
                   getDao().get_self(), 
                   getId(), 
                   memberID, 
                   links::TREASURER);

    Edge::getOrNew(getDao().get_self(),
                   getDao().get_self(),
                   memberID,
                   getId(),
                   links::TREASURER_OF);
}

void Treasury::checkTreasurerAuth() 
{
    //Contract account has treasury perms over all DAO's
    if (eosio::has_auth(getDao().get_self())) {
      return;
    }

    auto treasurerEdges = getDao()
                          .getGraph()
                          .getEdgesFrom(getId(), links::TREASURER);

    auto adminEdges = getDao()
                      .getGraph()
                      .getEdgesFrom(getDaoID(), hypha::common::ADMIN);
    
    treasurerEdges.insert(
        treasurerEdges.end(), 
        std::move_iterator(adminEdges.end()),
        std::move_iterator(adminEdges.begin())
    );
        
    EOS_CHECK(
      std::any_of(treasurerEdges.begin(), treasurerEdges.end(), [this](const Edge& edge) {
        Member member(getDao(), edge.to_node);
        return eosio::has_auth(member.getAccount());
      }),
      to_str("Only treasurers of the dao are allowed to perform this action")
    );
}

Treasury Treasury::getFromDaoID(dao& dao, uint64_t daoID)
{
    auto treasuryEdge = Edge::get(dao.get_self(), daoID, links::TREASURY);

    return Treasury(dao, treasuryEdge.getToNode());
}

}

#endif