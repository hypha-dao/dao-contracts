#pragma once

#include <cstdint>

namespace hypha {

class dao;

namespace pricing {

#ifdef USE_PRICING_PLAN

class PricingPlan;

void checkDaoCanEnrrollMember(dao& dao, uint64_t daoID);

/**
 * @brief Execute side effects of downgrading a DAO to a lower plan
 * 
 * @param dao 
 * @param daoID 
 */
void onDaoPlanChange(dao& dao, uint64_t daoID, PricingPlan& newPlan);
#endif

}
} // namespace hypha::pricing
