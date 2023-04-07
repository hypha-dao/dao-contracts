#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>

#include <array>
#include <common.hpp>
#include <member.hpp>
#include <util.hpp>
#include <proposals/payout_proposal.hpp>
#include <document_graph/content_wrapper.hpp>
#include <document_graph/content.hpp>
#include <logger/logger.hpp>
#include <typed_document.hpp>


namespace hypha
{

/**
 * @brief Payout proposal is used as a generic wrapper for different types of proposals
 * Contribution (Request for a payment)
 * Policy (Policy document)
 * Quest Start (start of a quest)
 * Quest Completion (Completion of Quest)
 * 
 */

std::optional<name> PayoutProposal::getSubType(ContentWrapper &contentWrapper)
{
    auto [_, subType] = contentWrapper.get(DETAILS, common::PAYOUT_SUBTYPE);

    return subType ? std::optional{subType->getAs<name>()} : std::nullopt;
}

bool PayoutProposal::isPayable(ContentWrapper &contentWrapper)
{
    if (auto subType = getSubType(contentWrapper))
    {
        //Check if it's a valid subtype
        return subType == common::QUEST_COMPLETION;
    }

    return true;
}

// void PayoutProposal::checkPayoutFields(const name &proposer, ContentWrapper &contentWrapper)
// {

// }

void PayoutProposal::checkPolicyFields(const name &proposer, ContentWrapper &contentWrapper)
{

}

void PayoutProposal::checkQuestStartFields(const name &proposer, ContentWrapper &contentWrapper)
{
    // PERIOD_COUNT - number of periods the quest is valid for
    auto periodCount = contentWrapper.getOrFail(DETAILS, PERIOD_COUNT);

    EOS_CHECK(
        periodCount->getAs<int64_t>() < 26, 
        PERIOD_COUNT + string(" must be less than 26. You submitted: ") + std::to_string(std::get<int64_t>(periodCount->value))
    );
}

void PayoutProposal::checkQuestCompletionFields(const name &proposer, ContentWrapper &contentWrapper)
{

}

void PayoutProposal::checkPaymentFields(const name& proposer, ContentWrapper &contentWrapper, bool onlyCoreMembers)
{
    auto detailsGroup = contentWrapper.getGroupOrFail(DETAILS);

    // recipient must exist and be a DHO member
    name recipient = contentWrapper.getOrFail(DETAILS, RECIPIENT)->getAs<eosio::name>();

    EOS_CHECK(
        proposer == recipient,
        "Proposer must be the recipient of the payment"
    );
    
    EOS_CHECK(
        Member::isMember(m_dao, m_daoID, recipient) ||
        !onlyCoreMembers && Member::isCommunityMember(m_dao, m_daoID, recipient), 
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
        EOS_CHECK(deferred <= 100, DEFERRED + string(" must be less than or equal to 100 (=100%). You submitted: ") + std::to_string(deferred));

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
    auto checkToken = [&](const Content& item, const char* tokenStr, const asset& tokenAsset, bool& tokenExists) {
        
        bool isToken = item.label == tokenStr;

        if (isToken) 
        {    
            EOS_CHECK(
                !tokenExists, 
                to_str("Multiple ", tokenStr, " items are not allowed")
            );

            EOS_CHECK(
                item.getAs<asset>().symbol == tokenAsset.symbol,
                to_str(tokenStr, 
                       " unexpected token symbol. Expected: ",
                       tokenAsset, " and got ", item.getAs<asset>())
            );
        }

        tokenExists = tokenExists || isToken;
    };

    bool hasPeg = false;
    bool hasVoice = false;
    bool hasReward = false;

    for (const auto& item : *detailsGroup) {
        checkToken(item, common::PEG_AMOUNT, tokens.peg, hasPeg);
        checkToken(item, common::VOICE_AMOUNT, tokens.voice, hasVoice);
        checkToken(item, common::REWARD_AMOUNT, tokens.reward, hasReward);
    }

    EOS_CHECK(
        hasPeg, 
        "Missing peg_amount item"
    );

    EOS_CHECK(
        hasVoice, 
        "Missing voice_amount item"
    );

    EOS_CHECK(
        hasReward, 
        "Missing reward_amount item"
    );
}
std::optional<Document> PayoutProposal::getParent(const char* parentItem, const name& parentType, ContentWrapper &contentWrapper, bool mustExist) 
{
    if (auto [_, parent] = contentWrapper.get(DETAILS, parentItem);
        parent) 
    {
        auto parentDoc = TypedDocument::withType(m_dao, parent->getAs<int64_t>(), parentType);
        return parentDoc;
    }

    EOS_CHECK(
        !mustExist,
        to_str("Required item not found: ", parentItem)
    )

    return std::nullopt;
}

Document PayoutProposal::getParent(uint64_t from, const name& edgeName, const name& parentType)
{
    auto parentEdge = Edge::get(m_dao.get_self(), from, edgeName);

    return TypedDocument::withType(m_dao, parentEdge.getToNode(), parentType);
}

void PayoutProposal::proposeImpl(const name &proposer, ContentWrapper &contentWrapper)
{
    TRACE_FUNCTION()

    //Check if this is an special payout proposal
    if (auto subType = getSubType(contentWrapper))
    {
        //Check if it's a valid subtype
        switch (*subType)
        {
        case common::QUEST_COMPLETION:
            checkQuestCompletionFields(proposer, contentWrapper);
            break;
        case common::QUEST_START:
            checkPaymentFields(proposer, contentWrapper, false);
            checkQuestStartFields(proposer, contentWrapper);
            break;
        case common::POLICY:
            checkPolicyFields(proposer, contentWrapper);
            break;
        default:
            EOS_CHECK(
                false, 
                to_str("Invalid subtype ", *subType)
            );
            break;
        }

        return;
    }

    //Then dealing with regular Payout proposal
    checkPaymentFields(proposer, contentWrapper, false);
}

void PayoutProposal::postProposeImpl(Document &proposal)
{
    TRACE_FUNCTION();

    auto cw = proposal.getContentWrapper();
    //Check if this is an special payout proposal
    if (auto subType = getSubType(cw))
    {
        //Check if it's a valid subtype
        switch (*subType)
        {
        case common::QUEST_COMPLETION: {

            //We must have stablished a quest start 
            auto questStartDoc = *getParent(common::QUEST_START_ITEM, common::QUEST_START, cw, true);

            //Check start quest hasn't been completed yet or there isn't an open completion proposal
            auto [isCompleted, completeEdge] = Edge::getIfExists(m_dao.get_self(), questStartDoc.getID(), common::COMPLETED_BY);
            
            EOS_CHECK(
                !isCompleted,
                to_str("Quest was already completed by: ", completeEdge.getToNode())
            );

            auto [isLocked, lockedEdge] = Edge::getIfExists(m_dao.get_self(), questStartDoc.getID(), common::LOCKED_BY);

            EOS_CHECK(
                !isLocked,
                to_str("Quest is already being completed by: ", lockedEdge.getToNode())
            );

            //Create edge to start quest
            Edge(m_dao.get_self(), m_dao.get_self(), proposal.getID(), questStartDoc.getID(), common::QUEST_START);
            Edge(m_dao.get_self(), m_dao.get_self(), questStartDoc.getID(), proposal.getID(), common::LOCKED_BY);

        }   break;
        case common::QUEST_START: {
            //We might have stablished a previous quest start

            if (auto parentQuest = getParent(common::PARENT_QUEST_ITEM, common::QUEST_START, cw)) {
                //Create parent quest edge
                Edge(m_dao.get_self(), m_dao.get_self(), proposal.getID(), parentQuest->getID(), common::PARENT_QUEST);
            }

            //Check start period
            auto startPeriod = cw.getOrFail(DETAILS, START_PERIOD);
            
            Document period = TypedDocument::withType(
                m_dao,
                static_cast<uint64_t>(startPeriod->getAs<int64_t>()),
                common::PERIOD
            );

            EOS_CHECK(
                Edge::exists(m_dao.get_self(), period.getID(), m_daoID, common::DAO),
                "Period must belong to the DAO"
            );

            Edge(m_dao.get_self(), m_dao.get_self(), proposal.getID(), period.getID(), common::START);

        }   break;
        case common::POLICY: {
            //We might have stablished a master policy
        }   break;
        default:
            EOS_CHECK(
                false, 
                to_str("Invalid subtype ", *subType)
            );
            break;
        }

        return;
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

    auto cw = proposal.getContentWrapper();

    auto subType = getSubType(cw);

    //Regular payout
    if (!subType) {
        pay(proposal, common::PAYOUT);
    }
    else if (*subType == common::QUEST_COMPLETION) {
        auto questStart = getParent(proposal.getID(), common::QUEST_START, common::QUEST_START);
        pay(questStart, common::QUEST_COMPLETION);
        Edge(m_dao.get_self(), m_dao.get_self(), questStart.getID(), proposal.getID(), common::COMPLETED_BY);
        Edge(m_dao.get_self(), m_dao.get_self(), proposal.getID(), questStart.getID(), common::COMPLETED);
    }
}

void PayoutProposal::pay(Document &proposal, eosio::name edgeName)
{
    TRACE_FUNCTION()

    ContentWrapper contentWrapper = proposal.getContentWrapper();

    // recipient must exist and be a DHO member
    name recipient = contentWrapper.getOrFail(DETAILS, RECIPIENT)->getAs<eosio::name>();

    Document recipientDoc(m_dao.get_self(), m_dao.getMemberID(recipient));

    Edge::write(m_dao.get_self(), m_dao.get_self(), recipientDoc.getID(), proposal.getID(), edgeName);

    std::string memo{"one-time payment on proposal: " + to_str(proposal.getID())};

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

void PayoutProposal::failImpl(Document &proposal)
{
    auto cw = proposal.getContentWrapper();

    if (auto subType = getSubType(cw); subType && *subType == common::QUEST_COMPLETION)
    {
        //Remove locked by edge for quest completion subtype so later on we can create a new completion proposal
        Edge::getTo(m_dao.get_self(), proposal.getID(), common::LOCKED_BY).erase();
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