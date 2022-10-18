#pragma once

#include <cstdint>

namespace hypha {

class dao;

namespace pricing {

class PricingPlan;

void checkDaoCanEnrrollMember(dao& dao, uint64_t daoID);

/**
 * @brief Execute side effects of downgrading a DAO to a lower plan
 * 
 * @param dao 
 * @param daoID 
 */
void onDaoPlanChange(dao& dao, uint64_t daoID, PricingPlan& newPlan);

}
} // namespace hypha::pricing
