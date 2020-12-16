#include <period.hpp>
#include <common.hpp>
#include <util.hpp>
#include <dao.hpp>
#include <document_graph/edge.hpp>
#include <member.hpp>
#include <assignment.hpp>

namespace hypha
{
    Assignment::Assignment(dao *dao, const eosio::checksum256 &hash) : Document(dao->get_self(), hash), m_dao{dao}, contentWrapper{getContentWrapper()}
    {
        auto [idx, docType] = contentWrapper.get(SYSTEM, TYPE);

        eosio::check(idx != -1, "Content item labeled 'type' is required for this document but not found.");
        eosio::check(docType->getAs<eosio::name>() == common::ASSIGNMENT,
                     "invalid document type. Expected: " + common::ASSIGNMENT.to_string() +
                         "; actual: " + docType->getAs<eosio::name>().to_string());
    }

    Member Assignment::getAssignee()
    {
        return Member (m_dao->get_self(), Edge::get(m_dao->get_self(), getHash(), common::ASSIGNEE_NAME).getToNode());
        // return m;
    }

    eosio::time_point Assignment::getApprovedTime()
    {
        return Edge::get(m_dao->get_self(), getAssignee().getHash(), common::ASSIGNED).getCreated();
    }

    bool Assignment::isClaimed(Period *period)
    {
        return Edge::exists(m_dao->get_self(), getHash(), period->getHash(), common::CLAIMED);
    }

    std::optional<Period> Assignment::getNextClaimablePeriod()
    {
        // Ensure that the claimed period is within the approved period count
        Period period(m_dao, contentWrapper.getOrFail(DETAILS, START_PERIOD)->getAs<eosio::checksum256>());
        int64_t periodCount = contentWrapper.getOrFail(DETAILS, PERIOD_COUNT)->getAs<int64_t>();
        int64_t counter = 0;

        while (counter < periodCount)
        {
            if (period.getStartTime().sec_since_epoch() >= getApprovedTime().sec_since_epoch() &&        // if period comes after assignment creation
                period.getEndTime().sec_since_epoch() > eosio::current_time_point().sec_since_epoch() && // if period has lapsed
                !isClaimed(&period))                                                                     // and not yet claimed
            {
                return std::optional<Period>{period};
            }
            period = period.next();
            counter++;
        }
        return std::nullopt;
    }

    eosio::asset Assignment::getAsset(const symbol *symbol, const std::string &key)
    {
        if (auto [idx, assetContent] = contentWrapper.get(DETAILS, key); assetContent)
        {
            eosio::check(std::holds_alternative<eosio::asset>(assetContent->value), "fatal error: expected token type must be an asset value type: " + assetContent->label);
            return std::get<eosio::asset>(assetContent->value);
        }
        return eosio::asset{0, *symbol};
    }

    eosio::asset Assignment::getSalaryAmount(const eosio::symbol *symbol)
    {
        switch (symbol->code().raw())
        {
        case common::S_HUSD.code().raw():
            return getAsset(symbol, HUSD_SALARY_PER_PERIOD);

        case common::S_HVOICE.code().raw():
            return getAsset(symbol, HVOICE_SALARY_PER_PERIOD);

        case common::S_HYPHA.code().raw():
            return getAsset(symbol, HYPHA_SALARY_PER_PERIOD);
        }

        return eosio::asset{0, *symbol};
    }

    eosio::asset Assignment::getSalaryAmount(const eosio::symbol *symbol, Period *period)
    {
        switch (symbol->code().raw())
        {

        case common::S_SEEDS.code().raw():
            return calcDSeedsSalary(period);
        }

        return eosio::asset{0, *symbol};
    }

    eosio::asset Assignment::calcDSeedsSalary(Period *period)
    {
        // If there is an explicit ESCROW SEEDS salary amount, support sending it; else it should be calculated
        if (auto [idx, seedsEscrowSalary] = contentWrapper.get(DETAILS, "seeds_escrow_salary_per_phase"); seedsEscrowSalary)
        {
            eosio::check(std::holds_alternative<eosio::asset>(seedsEscrowSalary->value), "fatal error: expected token type must be an asset value type: " + seedsEscrowSalary->label);
            return std::get<eosio::asset>(seedsEscrowSalary->value);
        }
        else if (auto [idx, usdSalaryValue] = contentWrapper.get(DETAILS, USD_SALARY_PER_PERIOD); usdSalaryValue)
        {
            eosio::check(std::holds_alternative<eosio::asset>(usdSalaryValue->value), "fatal error: expected token type must be an asset value type: " + usdSalaryValue->label);

            // Dynamically calculate the SEEDS amount based on the price at the end of the period being claimed
            return getSeedsAmount(m_dao->getSettingOrFail<int64_t>(SEEDS_DEFERRAL_FACTOR_X100),
                                  usdSalaryValue->getAs<eosio::asset>(),
                                  period->getEndTime(),
                                  (float)(contentWrapper.getOrFail(DETAILS, TIME_SHARE)->getAs<int64_t>() / (float)100),
                                  (float)(contentWrapper.getOrFail(DETAILS, DEFERRED)->getAs<int64_t>() / (float)100));
        }
        return asset{0, common::S_SEEDS};
    }

} // namespace hypha