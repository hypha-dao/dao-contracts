#include "pricing/plan_manager.hpp"
#include "pricing/common.hpp"

#include "pricing/billing_info.hpp"
#include "pricing/pricing_plan.hpp"
#include "pricing/price_offer.hpp"

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
        to_str(_s("Invalid DAO name: "), daoName)
    )

    EOS_CHECK(
        getDao().getGraph().getEdgesFrom(*daoId, links::PLAN_MANAGER).empty(),
        _s("DAOs can only have 1 Plan Manager linked")
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
    if (amount.amount == 0) return;

    EOS_CHECK(
        amount.amount > 0,
        _s("Amount to credit must be positive")
    )

    auto newCredit = getCredit() + amount;

    //New credit must be greater than previous one
    EOS_CHECK(
        newCredit.is_valid() && newCredit > getCredit(),
        _s("Credit amount has to be valid")
    )

    setCredit(std::move(newCredit));
}

void PlanManager::removeCredit(const asset& amount)
{
    if (amount.amount == 0) return;

    auto newCredit = getCredit() - amount;

    EOS_CHECK(
        amount.amount > 0,
        _s("Amount to substract from credit must be positive")
    )

    EOS_CHECK(
        newCredit.amount >= 0,
        _s("Insufficient funds in Plan Manager")
    )

    EOS_CHECK(
        newCredit.is_valid() && newCredit < getCredit(),
        _s("Credit amount has to be valid")
    )

    setCredit(std::move(newCredit));
}

std::optional<PlanManager> PlanManager::getFromDaoIfExists(dao& dao, uint64_t daoID)
{
    if (auto [exists, edge] = Edge::getIfExists(
        dao.get_self(),
        daoID,
        links::PLAN_MANAGER
    ); exists) {
        return PlanManager(dao, edge.getToNode());
    }

    return std::nullopt;
}

PlanManager PlanManager::getFromDaoID(dao& dao, uint64_t daoID)
{
    auto planManagerEdge = Edge::get(dao.get_self(), daoID, links::PLAN_MANAGER);
    return PlanManager(dao, planManagerEdge.getToNode());
}

PricingPlan PlanManager::getDefaultPlan(dao& dao)
{
    return PricingPlan(dao, Edge::get(dao.get_self(), 
                                      dao.getRootID(), 
                                      links::DEFAULT_PRICING_PLAN).getToNode()); 
}

PricingPlan PlanManager::getEcosystemPlan(dao& dao)
{
    return PricingPlan(dao, Edge::get(dao.get_self(), 
                                      dao.getRootID(), 
                                      links::ECOSYSTEM_PRICING_PLAN).getToNode()); 
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

BillingInfo PlanManager::getStartBill()
{
    return BillingInfo(getDao(), 
                       Edge::get(getDao().get_self(), 
                                 getId(), 
                                 links::START_BILL).getToNode());
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
    auto current = Edge::get(getDao().get_self(), getId(), links::CURRENT_BILL);

    eosio::asset credit = {0, hypha::common::S_HYPHA};

    auto next = std::make_unique<BillingInfo>(getDao(), current.getToNode());

    auto now = eosio::current_time_point();

    //Shouldn't happen, but we can omit if it's the last bill
    EOS_CHECK(
        now <= next->getEndDate() || !next->getNextBill(),
        "Current time must be less than current bill end date"
    )

    for (; next; next = next->getNextBill()) {
        
        auto start = next->getStartDate();
        auto end = next->getEndDate();

        auto total = (end - start).count();
        auto remaining = std::max((end - now).count(), int64_t(0));

        auto amount = adjustAsset(
            next->getTotalPaid(),
            std::min(1.0f, float(remaining) / float(total))
        );

        EOS_CHECK(
            amount.amount >= 0, 
            to_str("Amount must be positive: ", amount, " Bill: ", next->getId(), " ", now)
        )

        credit += amount;

        //now = end;
    }

    return credit;
}

bool PlanManager::hasBills()
{
    return !getDao()
            .getGraph()
            .getEdgesFrom(getId(), links::BILL).empty();
}

}