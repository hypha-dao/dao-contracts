#pragma once

#include <optional>

#include <typed_document.hpp>
#include <macros.hpp>

namespace hypha::pricing {

class BillingInfo;
class PricingPlan;

class PlanManager : public TypedDocument
{
    DECLARE_DOCUMENT(
        Data,
        PROPERTY(credit, eosio::asset, Credit, USE_GETSET),
        PROPERTY(type, std::string, Type, USE_GETSET)
    )
public:
    PlanManager(dao& dao, uint64_t id);
    PlanManager(dao& dao, name daoName, Data data);

    void setStartBill(const BillingInfo& bill);
    void setCurrentBill(const BillingInfo& bill);
    void setLastBill(const BillingInfo& bill);

    void addCredit(const asset& amount);
    void removeCredit(const asset& amount);

    BillingInfo getStartBill();
    BillingInfo getCurrentBill();
    BillingInfo getLastBill();

    bool hasBills();

    eosio::asset calculateCredit();

    static std::optional<PlanManager> getFromDaoIfExists(dao& dao, uint64_t daoID);

    static PlanManager getFromDaoID(dao& dao, uint64_t daoID);

    static PricingPlan getDefaultPlan(dao& dao);

    static PricingPlan getEcosystemPlan(dao& dao);
private:
    virtual const std::string buildNodeLabel(ContentGroups &content) override
    {
        return "Pricing Plan";
    }
};

using PlanManagerData = PlanManager::Data;

}