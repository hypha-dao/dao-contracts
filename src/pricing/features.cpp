#include "pricing/features.hpp"

#include "dao.hpp"

#include "pricing/plan_manager.hpp"
#include "pricing/pricing_plan.hpp"
#include "pricing/billing_info.hpp"
#include <document_graph/edge.hpp>

#include "member.hpp"

namespace hypha::pricing
{

static 
PricingPlan getCurrentPlan(PlanManager& planManager)
{
    auto currentBill = planManager.getCurrentBill();

    auto bill = std::make_unique<BillingInfo>(planManager.getDao(), currentBill.getId());

    auto now = eosio::current_time_point();

    while (bill) {
        auto next = bill->getNextBill();
        if (bill->getIsInfinite() ||
            now < bill->getEndDate() ||
            (now < bill->getExpirationDate() && !next)) {
            return bill->getPricingPlan();
        }
        bill = std::move(next);
    }

    return PlanManager::getDefaultPlan(planManager.getDao());
}

void checkDaoCanEnrrollMember(dao& dao, uint64_t daoID)
{
    if (auto planManager = PlanManager::getFromDaoIfExists(dao, daoID)) 
    {
        auto currentPlan = getCurrentPlan(*planManager);

        auto currentMembers = Edge::getEdgesFromCount(dao.get_self(), daoID, common::MEMBER);

        EOS_CHECK(
            currentMembers < currentPlan.getMaxMemberCount(),
            to_str("You already reached the max number of members available for your plan: ", currentPlan.getMaxMemberCount())
        )
    }
    //For DAO's without Plan Manager we assume there is no limit
}

void onDaoPlanChange(dao& dao, uint64_t daoID, PricingPlan& newPlan)
{
    //Check if we need to remove users
    auto membersEdges = dao.getGraph().getEdgesFrom(daoID, common::MEMBER);

    if (membersEdges.size() > newPlan.getMaxMemberCount()) {
        //We need to remove some users

        auto isAdmin = [&](const Edge& a) {
            return Edge::exists(dao.get_self(), daoID, a.to_node, common::ADMIN);
        };

        auto end = std::partition(
            membersEdges.begin(), 
            membersEdges.end(), 
            isAdmin
        );

        std::sort(membersEdges.begin(), end, 
                  [&isAdmin](const Edge& a, const Edge& b) {
                      return a.created_date < b.created_date;
                  });

        std::sort(end, membersEdges.end(),
                  [&isAdmin](const Edge& a, const Edge& b) {
                      return a.created_date < b.created_date;
                  });

        auto memEdgeIt = membersEdges.begin() +  newPlan.getMaxMemberCount();
        
        while (memEdgeIt != membersEdges.end()) {
            Member mem(dao, memEdgeIt->to_node);

            //If it's admin or enroller let's remove those perms as well
            if (Edge::exists(dao.get_self(), daoID, mem.getID(), common::ADMIN))
            {
                Edge::get(dao.get_self(), daoID, mem.getID(), common::ADMIN).erase();
            }

            if (Edge::exists(dao.get_self(), daoID, mem.getID(), common::ENROLLER))
            {
                Edge::get(dao.get_self(), daoID, mem.getID(), common::ENROLLER).erase();
            }

            mem.removeMembershipFromDao(daoID);
            mem.apply(daoID, "Auto apply after being removed");

            ++memEdgeIt;
        }
    }
}

} // namespace hypha::pricing