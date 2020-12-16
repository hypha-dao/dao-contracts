#include <util.hpp>
#include <eosio/crypto.hpp>
#include <document_graph/document.hpp>
#include <document_graph/content_wrapper.hpp>
#include <common.hpp>

#include <cmath>

namespace hypha
{

    eosio::checksum256 getRoot(const eosio::name &contract)
    {
        ContentGroups cgs = Document::rollup(Content(ROOT_NODE, contract));
        return Document::hashContents(cgs);
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

    eosio::asset getSeedsAmount(int64_t deferralFactor,
                                const eosio::asset &usd_amount,
                                const eosio::time_point &price_time_point,
                                const float &time_share,
                                const float &deferred_perc)
    {
        asset adjusted_usd_amount = adjustAsset(adjustAsset(usd_amount, deferred_perc), time_share);
        float seeds_deferral_coeff = (float) deferralFactor / (float)100;
        float seeds_price = getSeedsPriceUsd(price_time_point);
        return adjustAsset(asset{static_cast<int64_t>(adjusted_usd_amount.amount * (float)100 * (float)seeds_deferral_coeff),
                                 common::S_SEEDS},
                           (float)1 / (float)seeds_price);
    }

} // namespace hypha