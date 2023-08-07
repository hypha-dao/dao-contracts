#include <util.hpp>
#include <eosio/crypto.hpp>
#include <document_graph/document.hpp>
#include <document_graph/content_wrapper.hpp>
#include <common.hpp>
#include <logger/logger.hpp>

#include <cmath>
#include <charconv>

namespace hypha
{
    AssetBatch& operator+=(AssetBatch& self, const AssetBatch& other)
    {
      self.peg += other.peg;
      self.reward += other.reward;
      self.voice += other.voice;
      return self;  
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
        
        salaries.peg = denormalizeToken(pegSalaryPerPeriod * salaryConf.pegMultipler, tokens.peg);

        double rewardSalaryPerPeriod = (salaryConf.periodSalary * salaryConf.deferredPerc) / salaryConf.rewardToPegRatio;

        salaries.reward = denormalizeToken(rewardSalaryPerPeriod * salaryConf.rewardMultipler, tokens.reward);

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

    ContentGroups getDAOContent(const eosio::name &dao_name, string daoType)
    {
        ContentGroups cgs ({
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, DETAILS), 
                Content(DAO_NAME, dao_name),
                //All DAO's are Individual type by default
                Content{common::DAO_TYPE, std::move(daoType)}
            },
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, SYSTEM), 
                Content(TYPE, common::DAO), 
                Content(NODE_LABEL, dao_name.to_string())}});

        return std::move(cgs);
    }

    eosio::asset adjustAsset(const asset &originalAsset, const float &adjustment)
    {
        return eosio::asset{static_cast<int64_t>(originalAsset.amount * adjustment), originalAsset.symbol};
    }

    vector<string_view> splitStringView(string_view str, char delimiter)
    {
        vector<string_view> strings;
        size_t start = 0;
        
        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == delimiter) {
                if (start != i) { //Avoid creating empty substrings
                    strings.push_back(str.substr(start, i - start));
                }
                start = i + 1;
            }
        }
        if (start != str.size()) { //Avoid creating empty substrings
            strings.push_back(str.substr(start));
        }
        return strings;
    }

    int64_t stringViewToInt(string_view str)
    {
        int64_t val;
        auto [ptr, err] = std::from_chars(str.data(), str.data() + str.size(), val);
        EOS_CHECK(err != std::errc::invalid_argument && 
                  err != std::errc::result_out_of_range,
                  to_str("Cannot conver string to int: ", std::string(str)))
        return val;
    }

    uint64_t stringViewToUInt(string_view str)
    {
        uint64_t val;
        auto [ptr, err] = std::from_chars(str.data(), str.data() + str.size(), val);
        EOS_CHECK(err != std::errc::invalid_argument && 
                  err != std::errc::result_out_of_range,
                  to_str("Cannot conver string to int: ", std::string(str)))
        return val;
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