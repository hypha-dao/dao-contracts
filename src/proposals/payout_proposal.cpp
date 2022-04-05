#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>

#include <common.hpp>
#include <member.hpp>
#include <util.hpp>
#include <proposals/payout_proposal.hpp>
#include <document_graph/content_wrapper.hpp>
#include <document_graph/content.hpp>
#include <logger/logger.hpp>


namespace hypha
{

    void PayoutProposal::proposeImpl(const name &proposer, ContentWrapper &contentWrapper)
    {
        TRACE_FUNCTION()
        auto detailsGroup = contentWrapper.getGroupOrFail(DETAILS);

        // recipient must exist and be a DHO member
        name recipient = contentWrapper.getOrFail(DETAILS, RECIPIENT)->getAs<eosio::name>();
        
        EOS_CHECK(
            Member::isMember(m_dao, m_daoID, recipient), 
            "only members are eligible for payouts: " + recipient.to_string()
        );

        auto tokens = AssetBatch {
            .reward = m_daoSettings->getOrFail<asset>(common::REWARD_TOKEN),
            .peg = m_daoSettings->getOrFail<asset>(common::PEG_TOKEN),
            .voice = m_daoSettings->getOrFail<asset>(common::VOICE_TOKEN)
        };

        // if usd_amount is provided in the DETAILS section, convert that to token components
        //  (deferred_perc_x100 will be required)
        if (auto [idx, usdAmount] = contentWrapper.get(DETAILS, USD_AMOUNT); usdAmount)
        {
            EOS_CHECK(std::holds_alternative<eosio::asset>(usdAmount->value),
                         "fatal error: expected token type must be an asset value type: " + usdAmount->label);
            eosio::asset usd = std::get<eosio::asset>(usdAmount->value);

            // deferred_x100 is required and must be greater than or equal to zero and less than or equal to 10000
            int64_t deferred = contentWrapper.getOrFail(DETAILS, DEFERRED)->getAs<int64_t>();
            EOS_CHECK(deferred >= 0, DEFERRED + string(" must be greater than or equal to zero. You submitted: ") + std::to_string(deferred));
            EOS_CHECK(deferred <= 10000, DEFERRED + string(" must be less than or equal to 10000 (=100%). You submitted: ") + std::to_string(deferred));

            auto rewardPegVal = m_daoSettings->getOrFail<eosio::asset>(common::REWARD_TO_PEG_RATIO);
            
            auto salaries = calculateSalaries(SalaryConfig {
                .periodSalary = normalizeToken(usd),
                .rewardToPegRatio = normalizeToken(rewardPegVal),
                .deferredPerc = deferred / 100.0,
                .voiceMultipler = 2.0 
            }, tokens);

            ContentWrapper::insertOrReplace(*detailsGroup, Content{ common::PEG_AMOUNT, salaries.peg });
            ContentWrapper::insertOrReplace(*detailsGroup, Content{ common::VOICE_AMOUNT, salaries.voice });
            ContentWrapper::insertOrReplace(*detailsGroup, Content{ common::REWARD_AMOUNT, salaries.reward });
        }

        //Verify there is only 1 item of each token type
        bool hasPeg = false;
        bool hasVoice = false;
        bool hasReward = false;

        for (const auto& item : *detailsGroup) {

            bool isPeg = item.label == common::PEG_AMOUNT;

            if (isPeg) {
                
                EOS_CHECK(
                    !hasPeg, 
                    util::to_str("Multiple peg_amount items are not allowed")
                )

                EOS_CHECK(
                    item.getAs<asset>().symbol == tokens.peg.symbol,
                    util::to_str(common::PEG_AMOUNT, 
                                 " unexpected token symbol. Expected: ",
                                 tokens.peg)
                )
            }
            
            bool isVoice = item.label == common::VOICE_AMOUNT;

            if (isVoice) {
                
                EOS_CHECK(
                    !hasVoice, 
                    util::to_str("Multiple voice_amount items are not allowed")
                )

                EOS_CHECK(
                    item.getAs<asset>().symbol == tokens.voice.symbol,
                    util::to_str(common::VOICE_AMOUNT, 
                                 " unexpected token symbol. Expected: ",
                                 tokens.voice)
                )
            }

            bool isReward = item.label == common::REWARD_AMOUNT;

            if (isReward) {
                
                EOS_CHECK(
                    !hasReward, 
                    util::to_str("Multiple reward_amount items are not allowed")
                )

                EOS_CHECK(
                    item.getAs<asset>().symbol == tokens.reward.symbol,
                    util::to_str(common::REWARD_AMOUNT, 
                                 " unexpected token symbol. Expected: ",
                                 tokens.reward)
                )
            }
            

            hasPeg = hasPeg || isPeg;
            hasVoice = hasVoice || isVoice;
            hasReward = hasReward || isReward;
        }
    }

    void PayoutProposal::passImpl(Document &proposal)
    {
        TRACE_FUNCTION()
        // Graph updates:
        //  dao     ---- payout ---->   payout
        //  member  ---- payout ---->   payout
        //  makePayment also creates edges from payout and the member to the individual payments
        Edge::write(m_dao.get_self(), m_dao.get_self(), m_daoID, proposal.getID(), common::PAYOUT);

        pay(proposal, common::PAYOUT);
    }

    void PayoutProposal::pay(Document &proposal, eosio::name edgeName)
    {
        TRACE_FUNCTION()

        ContentWrapper contentWrapper = proposal.getContentWrapper();

        // recipient must exist and be a DHO member
        name recipient = contentWrapper.getOrFail(DETAILS, RECIPIENT)->getAs<eosio::name>();

        Document recipientDoc(m_dao.get_self(), m_dao.getMemberID(recipient));

        Edge::write(m_dao.get_self(), m_dao.get_self(), recipientDoc.getID(), proposal.getID(), edgeName);

        std::string memo{"one-time payment on proposal: " + util::to_str(proposal.getID())};

        auto tokens = AssetBatch {
            .reward = m_daoSettings->getOrFail<asset>(common::REWARD_TOKEN),
            .peg = m_daoSettings->getOrFail<asset>(common::PEG_TOKEN),
            .voice = m_daoSettings->getOrFail<asset>(common::VOICE_TOKEN)
        };

        auto detailsGroup = contentWrapper.getGroupOrFail(DETAILS);

        auto payoutItems = std::vector<std::string>{
            common::VOICE_AMOUNT, 
            common::REWARD_AMOUNT, 
            common::PEG_AMOUNT
        };

        for (Content &content : *detailsGroup)
        {
            if (auto it = std::find(payoutItems.begin(), payoutItems.end(), content.label);
                it != payoutItems.end())
            {
                m_dao.makePayment(m_daoSettings, proposal.getID(), recipient, std::get<eosio::asset>(content.value), memo, eosio::name{0}, tokens);
                payoutItems.erase(it);
            }
        }
    }

    std::string PayoutProposal::getBallotContent(ContentWrapper &contentWrapper)
    {
        TRACE_FUNCTION()
        return contentWrapper.getOrFail(DETAILS, TITLE)->getAs<std::string>();
    }

    name PayoutProposal::getProposalType()
    {
        return common::PAYOUT;
    }
} // namespace hypha