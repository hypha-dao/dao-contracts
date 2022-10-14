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

        auto currentMembers = dao.getGraph().getEdgesFrom(daoID, common::MEMBER);

        EOS_CHECK(
            currentMembers.size() < currentPlan.getMaxMemberCount(),
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

        std::sort(membersEdges.begin(), membersEdges.end(), 
                  [&isAdmin](const Edge& a, const Edge& b) {
                      return (isAdmin(a) && !isAdmin(b)) ||
                             a.created_date < b.created_date;
                  });
        
        //auto beg = member

        std::string str;
        for (auto& m : membersEdges) {
            str += to_str(m.to_node, "\n");
        }
        EOS_CHECK(false, str);

        auto memEdgeIt = membersEdges.begin() +  newPlan.getMaxMemberCount();
        while (memEdgeIt != membersEdges.end()) {
            Member mem(dao, memEdgeIt->to_node);
            mem.removeMembershipFromDao(daoID);
            mem.apply(daoID, "Auto apply after being removed");
        }
    }
}

} // namespace hypha::pricing