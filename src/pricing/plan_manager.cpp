#include "pricing/plan_manager.hpp"
#include "pricing/common.hpp"

#include "pricing/billing_info.hpp"

#include <dao.hpp>

namespace hypha::pricing {

using namespace common;

PlanManager::PlanManager(dao& dao, uint64_t id)
    : TypedDocument(dao, id, types::PLAN_MANAGER)
{

}

PlanManager::PlanManager(dao& dao, name daoName, Data data)
    : TypedDocument(dao, types::PLAN_MANAGER)
{
    auto cgs = convert(std::move(data));

    initializeDocument(dao, cgs);

    auto daoId = getDao().getDAOID(daoName);

    EOS_CHECK(
        daoId, 
        to_str("Invalid DAO name: ", daoName)
    )

    EOS_CHECK(
        getDao().getGraph().getEdgesFrom(*daoId, links::PLAN_MANAGER).empty(),
        "DAOs can only have 1 Plan Manager linked"
    )

    Edge(
        getDao().get_self(),
        getDao().get_self(),
        *daoId,
        getId(),
        links::PLAN_MANAGER
    );
}

void PlanManager::addCredit(const asset& amount)
{
    EOS_CHECK(
        amount.amount > 0,
        "Amount to credit must be positive"
    )

    auto newCredit = getCredit() + amount;

    //New credit must be greater than previous one
    EOS_CHECK(
        newCredit.is_valid() && newCredit > getCredit(),
        "Credit amount has to be valid"
    )

    setCredit(std::move(newCredit));
}

void PlanManager::removeCredit(const asset& amount)
{
    auto newCredit = getCredit() - amount;

    EOS_CHECK(
        amount.amount > 0,
        "Amount to substract from credit must be positive"
    )

    EOS_CHECK(
        newCredit.amount >= 0,
        "Insufficient funds in Plan Manager"
    )

    EOS_CHECK(
        newCredit.is_valid() && newCredit < getCredit(),
        "Credit amount has to be valid"
    )

    setCredit(std::move(newCredit));
}

PlanManager PlanManager::getFromDaoID(dao& dao, uint64_t dao_id)
{
    auto planManagerEdge = Edge::get(dao.get_self(), dao_id, links::PLAN_MANAGER);
    return PlanManager(dao, planManagerEdge.getToNode());
}

void PlanManager::setStartBill(const BillingInfo& bill)
{
    EOS_CHECK(
        getDao().getGraph().getEdgesFrom(getId(), links::START_BILL).empty(),
        "Start billing info cannot be set twice"
    )

    Edge(
        getDao().get_self(),
        getDao().get_self(),
        getId(),
        bill.getId(),
        links::START_BILL
    );
}

void PlanManager::setCurrentBill(const BillingInfo& bill)
{
    if (auto [exists, edge] = Edge::getIfExists(getDao().get_self(), getId(), links::CURRENT_BILL); exists) {
        edge.erase();
    }

    Edge(
        getDao().get_self(),
        getDao().get_self(),
        getId(),
        bill.getId(),
        links::CURRENT_BILL
    );
}

void PlanManager::setLastBill(const BillingInfo& bill)
{
    if (auto [exists, edge] = Edge::getIfExists(getDao().get_self(), getId(), links::LAST_BILL); exists) {
        edge.erase();
    }

    Edge(
        getDao().get_self(),
        getDao().get_self(),
        getId(),
        bill.getId(),
        links::LAST_BILL
    );
}

BillingInfo PlanManager::getCurrentBill() 
{
    return BillingInfo(getDao(), 
                       Edge::get(getDao().get_self(), 
                                 getId(), 
                                 links::CURRENT_BILL).getToNode());
}

BillingInfo PlanManager::getLastBill()
{
    return BillingInfo(getDao(), 
                       Edge::get(getDao().get_self(), 
                                 getId(), 
                                 links::LAST_BILL).getToNode());
}

eosio::asset PlanManager::calculateCredit()
{
    return {};
}

bool PlanManager::hasBills()
{
    return !getDao()
            .getGraph()
            .getEdgesFrom(getId(), links::BILL).empty();
}

}