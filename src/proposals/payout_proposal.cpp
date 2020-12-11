#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>

#include <common.hpp>
#include <member.hpp>
#include <util.hpp>
#include <proposals/payout_proposal.hpp>
#include <document_graph/content_wrapper.hpp>
#include <document_graph/content.hpp>

namespace hypha
{

    void PayoutProposal::propose_impl(const name &proposer, ContentWrapper &contentWrapper)
    {

        auto detailsGroup = contentWrapper.getGroupOrFail(DETAILS);

        // if end_period is provided, use that for the price timestamp, but
        // default the timepoint to now
        eosio::time_point seedsPriceTimePoint = eosio::current_block_time();

        // TODO: need to confirm about usage of period_id vs hash
        if (auto [idx, endPeriod] = contentWrapper.get(DETAILS, END_PERIOD); endPeriod)
        {
            eosio::check(std::holds_alternative<int64_t>(endPeriod->value),
                         "fatal error: expected to be a uint64_t type: " + endPeriod->label);
            int64_t periodID = std::get<int64_t>(endPeriod->value);

            dao::period_table period_t(m_dao.get_self(), m_dao.get_self().value);
            auto p_itr = period_t.find((uint64_t)periodID);
            eosio::check(p_itr != period_t.end(), "period_id is not found: " + std::to_string(periodID));
            seedsPriceTimePoint = p_itr->end_time;
        }

       // eosio::print ("completed processing of optional end period");

        // if usd_amount is provided in the DETAILS section, convert that to token components
        //  (deferred_perc_x100 will be required)
        if (auto [idx, usdAmount] = contentWrapper.get(DETAILS, USD_AMOUNT); usdAmount)
        {
            eosio::check(std::holds_alternative<eosio::asset>(usdAmount->value),
                         "fatal error: expected token type must be an asset value type: " + usdAmount->label);
            eosio::asset usd = std::get<eosio::asset>(usdAmount->value);

            // deferred_x100 is required and must be greater than or equal to zero and less than or equal to 10000
            int64_t deferred = contentWrapper.getOrFail(DETAILS, DEFERRED)->getAs<int64_t>();
            eosio::check(deferred >= 0, DEFERRED + string(" must be greater than or equal to zero. You submitted: ") + std::to_string(deferred));
            eosio::check(deferred <= 10000, DEFERRED + string(" must be less than or equal to 10000 (=100%). You submitted: ") + std::to_string(deferred));

            Content husd(HUSD_AMOUNT, calculateHusd(usd, deferred));
            Content hypha(HYPHA_AMOUNT, calculateHypha(usd, deferred));
            Content hvoice(HVOICE_AMOUNT, asset{usd.amount, common::S_HVOICE});
            Content seeds(ESCROW_SEEDS_AMOUNT, m_dao.getSeedsAmount(usd,
                                                                    seedsPriceTimePoint,
                                                                    (float)1.00000000,
                                                                    (float)deferred / (float)100));

            ContentWrapper::insertOrReplace(*detailsGroup, husd);
            ContentWrapper::insertOrReplace(*detailsGroup, hypha);
            ContentWrapper::insertOrReplace(*detailsGroup, hvoice);
            ContentWrapper::insertOrReplace(*detailsGroup, seeds);
        }
        // else
        // {
        //     // if we wanted to verify the assets provided on a payout proposal,
        //     // we would do that here. But for now, we will just allow them to flow through as provided.

        //     // iterate through the details group looking for assets
        //     for (Content &content : *detailsGroup)
        //     {
        //         if (std::holds_alternative<eosio::asset>(content.value))
        //         {
        //             // do some checking on the asset provided
        //         }
        //     }
        // }
    }

    void PayoutProposal::pass_impl(Document &proposal)
    {
        // Graph updates:
        //  dho     ---- payout ---->   payout
        //  member  ---- payout ---->   payout 
        //  makePayment also creates edges from payout and the member to the individual payments

        Edge::write(m_dao.get_self(), m_dao.get_self(), getRoot(m_dao.get_self()), proposal.getHash(), common::PAYOUT);

        ContentWrapper contentWrapper = proposal.getContentWrapper();

        // recipient must exist and be a DHO member
        name recipient = contentWrapper.getOrFail(DETAILS, RECIPIENT)->getAs<eosio::name>();
        eosio::check(Member::isMember(m_dao.get_self(), recipient), "only members are eligible for payouts: " + recipient.to_string());

        Edge::write(m_dao.get_self(), m_dao.get_self(), Member::getHash(recipient), proposal.getHash(), common::PAYOUT);

        std::string memo{"one-time payment on proposal: " + readableHash(proposal.getHash())};
        auto detailsGroup = contentWrapper.getGroupOrFail(DETAILS);
        for (Content &content : *detailsGroup)
        {
            if (std::holds_alternative<eosio::asset>(content.value))
            {
                if (content.label == ESCROW_SEEDS_AMOUNT)
                {
                    m_dao.makePayment(proposal.getHash(), recipient, std::get<eosio::asset>(content.value), memo, common::ESCROW);
                }
                else
                {
                    m_dao.makePayment(proposal.getHash(), recipient, std::get<eosio::asset>(content.value), memo, eosio::name{0});
                }
            }
        }
    }

    std::string PayoutProposal::GetBallotContent(ContentWrapper &contentWrapper)
    {
        return contentWrapper.getOrFail(DETAILS, TITLE)->getAs<std::string>();
    }

    name PayoutProposal::GetProposalType()
    {
        return common::PAYOUT;
    }

    asset PayoutProposal::calculateHusd(const asset &usd, const int64_t &deferred)
    {
        asset nonDeferredUsd = adjustAsset(usd, (float)1 - ((float)deferred / (float)100));
        // convert symbol from USD to HUSD
        return asset{nonDeferredUsd.amount, common::S_HUSD};
    }

    asset PayoutProposal::calculateHypha(const asset &usd, const int64_t &deferred)
    {
        // calculate HYPHA phase salary amount
        float hypha_deferral_coeff = (float)m_dao.getSettingOrFail<int64_t>(HYPHA_DEFERRAL_FACTOR) / (float)100;
        asset deferredUsd = adjustAsset(usd, (float)(float)deferred / (float)100);
        return adjustAsset(asset{deferredUsd.amount, common::S_HYPHA}, hypha_deferral_coeff);
    }
} // namespace hypha