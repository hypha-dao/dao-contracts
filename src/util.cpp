#include <util.hpp>
#include <eosio/crypto.hpp>
#include <document_graph/document.hpp>
#include <document_graph/content_wrapper.hpp>
#include <common.hpp>
#include <logger/logger.hpp>

#include <cmath>

namespace hypha
{

    eosio::checksum256 getRoot(const eosio::name &contract)
    {
        ContentGroups cgs = getRootContent (contract);
        return Document::hashContents(cgs);
    }

    eosio::checksum256 getDAO(const eosio::name &dao_name)
    {
        ContentGroups cgs = getDAOContent (dao_name);
        return Document::hashContents(cgs);
    }

    float getPhaseToYearRatio(Settings* daoSettings)
    {
        int64_t periodDurationSec = daoSettings->getOrFail<int64_t>(common::PERIOD_DURATION);

        return getPhaseToYearRatio(daoSettings, periodDurationSec);       
    }

    float getPhaseToYearRatio(Settings* daoSettings, int64_t periodDuration)
    {
        return static_cast<float>(periodDuration) / common::YEAR_DURATION_SEC;
    }

    AssetBatch calculateSalaries(const SalaryConfig& salaryConf, const AssetBatch& tokens)
    {
        AssetBatch salaries;

        double pegSalaryPerPeriod = salaryConf.periodSalary * (1.0 - salaryConf.deferredPerc);
        
        salaries.peg = denormalizeToken(pegSalaryPerPeriod, tokens.peg);

        double rewardSalaryPerPeriod = (salaryConf.periodSalary * salaryConf.deferredPerc) / salaryConf.rewardToPegRatio;

        salaries.reward = denormalizeToken(rewardSalaryPerPeriod, tokens.reward);

        double voiceSalaryPerPeriod = salaryConf.periodSalary * salaryConf.voiceMultipler;
        //TODO: Make the multipler configurable
        salaries.voice = denormalizeToken(voiceSalaryPerPeriod, tokens.voice);

        return salaries;
    }

    ContentGroups getRootContent(const eosio::name &contract)
    {
        ContentGroups cgs ({
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, DETAILS), 
                Content(ROOT_NODE, contract)}, 
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, SYSTEM), 
                Content(TYPE, common::DHO),
                Content(NODE_LABEL, "Hypha DHO Root")}});

        return std::move(cgs);
    }

    ContentGroups getDAOContent(const eosio::name &dao_name)
    {
        ContentGroups cgs ({
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, DETAILS), 
                Content(DAO_NAME, dao_name)}, 
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, SYSTEM), 
                Content(TYPE, common::DAO), 
                Content(NODE_LABEL, dao_name)}});

        return std::move(cgs);
    }

    eosio::asset adjustAsset(const asset &originalAsset, const float &adjustment)
    {
        return eosio::asset{static_cast<int64_t>(originalAsset.amount * adjustment), originalAsset.symbol};
    }

    // retrieve the seeds price as of a specific point in time
    float getSeedsPriceUsd(const eosio::time_point &price_time_point)
    {
        price_history_tables ph_t(name("tlosto.seeds"), name("tlosto.seeds").value);

        // start at the end and decrement until the input time point is greater than the date on the price history table
        // assume price history table is in proper sequence; it does not have an index on the date
        auto ph_itr = ph_t.rbegin();
        while (ph_itr->id > ph_t.begin()->id &&
               price_time_point.sec_since_epoch() < ph_itr->date.sec_since_epoch())
        {
            ph_itr++;
        }

        asset seeds_usd = ph_itr->seeds_usd;
        float seeds_usd_float = (float)seeds_usd.amount / (float)pow(10, seeds_usd.symbol.precision());
        return (float)1 / (seeds_usd_float);
    }

    // get the current SEEDS price
    float getSeedsPriceUsd()
    {
        configtables c_t(name("tlosto.seeds"), name("tlosto.seeds").value);
        configtable config_t = c_t.get();

        float seeds_price_usd = (float)1 / ((float)config_t.seeds_per_usd.amount / (float)10000);
        return (float)1 / ((float)config_t.seeds_per_usd.amount / (float)config_t.seeds_per_usd.symbol.precision());
    }

    void issueToken(const eosio::name &token_contract,
                    const eosio::name &issuer,
                    const eosio::name &to,
                    const eosio::asset &token_amount,
                    const string &memo)
    {
        TRACE_FUNCTION()
        
        eosio::action(
            eosio::permission_level{issuer, eosio::name("active")},
            token_contract, eosio::name("issue"),
            std::make_tuple(issuer, token_amount, memo))
            .send();

        eosio::action(
            eosio::permission_level{issuer, eosio::name("active")},
            token_contract, eosio::name("transfer"),
            std::make_tuple(issuer, to, token_amount, memo))
            .send();
    }

    void issueTenantToken(const eosio::name &token_contract,
                          const eosio::name &tenant,
                          const eosio::name &issuer,
                          const eosio::name &to,
                          const eosio::asset &token_amount,
                          const string &memo)
    {
        TRACE_FUNCTION()

        eosio::action(
            eosio::permission_level{issuer, eosio::name("active")},
            token_contract, eosio::name("issue"),
            std::make_tuple(tenant, issuer, token_amount, memo))
            .send();

        eosio::action(
            eosio::permission_level{issuer, eosio::name("active")},
            token_contract, eosio::name("transfer"),
            std::make_tuple(tenant, issuer, to, token_amount, memo))
            .send();
    }

    double normalizeToken(const eosio::asset& token)
    {
      auto power = ::pow(10, token.symbol.precision());
      return static_cast<double>(token.amount) / power;
    }

    eosio::asset denormalizeToken(double amountNormalized, const eosio::asset& token)
    {
      auto power = ::pow(10, token.symbol.precision());
      return eosio::asset{static_cast<int64_t>(amountNormalized * power), token.symbol};
    }

    int64_t getTokenUnit(const eosio::asset& token)
    {
      auto symb =  token.symbol;

      return static_cast<int64_t>(::powf(10, symb.precision()));;
    }

} // namespace hypha