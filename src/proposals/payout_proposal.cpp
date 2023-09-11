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

void PayoutProposal::checkTokenItems(Settings* daoSettings, ContentWrapper contentWrapper)
{
    auto detailsGroup = contentWrapper.getGroupOrFail(DETAILS);
    
    auto tokens = AssetBatch {
        .reward = daoSettings->getSettingOrDefault<asset>(common::REWARD_TOKEN),
        .peg = daoSettings->getSettingOrDefault<asset>(common::PEG_TOKEN),
        .voice = daoSettings->getOrFail<asset>(common::VOICE_TOKEN)
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

        auto rewardPegVal = tokens.reward.is_valid() ?
             daoSettings->getOrFail<eosio::asset>(common::REWARD_TO_PEG_RATIO) : 
             eosio::asset{};
        
        auto salaries = calculateSalaries(SalaryConfig {
            .periodSalary = normalizeToken(usd),
            .rewardToPegRatio = normalizeToken(rewardPegVal),
            .deferredPerc = deferred / 100.0,
            .voiceMultipler = getMultiplier(daoSettings, common::VOICE_MULTIPLIER, 2.0),
            .rewardMultipler = getMultiplier(daoSettings, common::REWARD_MULTIPLIER, 1.0),
            .pegMultipler = getMultiplier(daoSettings, common::PEG_MULTIPLIER, 1.0)
        }, tokens);

        ContentWrapper::insertOrReplace(*detailsGroup, Content{ common::VOICE_AMOUNT, salaries.voice });
        if (tokens.peg.is_valid()) ContentWrapper::insertOrReplace(*detailsGroup, Content{ common::PEG_AMOUNT, salaries.peg });
        if (tokens.reward.is_valid()) ContentWrapper::insertOrReplace(*detailsGroup, Content{ common::REWARD_AMOUNT, salaries.reward });
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
        checkToken(item, common::VOICE_AMOUNT, tokens.voice, hasVoice);
        if (tokens.peg.is_valid()) checkToken(item, common::PEG_AMOUNT, tokens.peg, hasPeg);
        if (tokens.reward.is_valid()) checkToken(item, common::REWARD_AMOUNT, tokens.reward, hasReward);
    }

    EOS_CHECK(
        hasVoice,
        "Missing voice_amount item"
    );

    //Either the peg token was found, or the DAO doesn't have peg token
    EOS_CHECK(
        hasPeg || !tokens.peg.is_valid(), 
        "Missing peg_amount item"
    );

    //Either the reward token was found, or the DAO doesn't have reward token
    EOS_CHECK(
        hasReward || !tokens.reward.is_valid(),
        "Missing reward_amount item"
    );
}

bool PayoutProposal::checkMembership(const eosio::name& proposer, ContentGroups &contentGroups)
{
    return  Proposal::checkMembership(proposer, contentGroups) ||
            Member::isCommunityMember(m_dao, m_daoID, proposer);
}

void PayoutProposal::proposeImpl(const name &proposer, ContentWrapper &contentWrapper)
{
    TRACE_FUNCTION()

    // recipient must exist and be a DHO member
    name recipient = contentWrapper.getOrFail(DETAILS, RECIPIENT)->getAs<eosio::name>();

    EOS_CHECK(
        proposer == recipient,
        "Proposer must be the recipient of the payment"
    );

    checkTokenItems(m_daoSettings, contentWrapper);
}

void PayoutProposal::markRelatives(const name& link, const name& fromRelationship, const name& toRelationship, uint64_t docId, bool clear)
{
    auto current = docId;
    
    const auto getEdge = [&](int64_t doc) -> std::optional<Edge> {
        if (auto [exists, edge] = Edge::getIfExists(m_dao.get_self(), doc, link); exists) {
            return edge;
        }

        return std::nullopt;
    };

    while(auto parentEdge = getEdge(current)) {
        
        auto parentId = parentEdge->getToNode();

        //Alway point to the original docId and iterate every relative
        //If clear then remove the relationship
        if (clear) {
            //For now just clear parent relationship
            if (fromRelationship) {
                Edge::get(m_dao.get_self(), docId, parentId, fromRelationship).erase();
            }
            
            if (toRelationship) {
                Edge::get(m_dao.get_self(), parentId, docId, toRelationship).erase();
            }
        }
        else {
            //Hack to enable just creating either from or to relationship or both
            if (fromRelationship) {
                Edge(m_dao.get_self(), m_dao.get_self(), docId, parentId, fromRelationship);
            }
            if (toRelationship) {
                Edge(m_dao.get_self(), m_dao.get_self(), parentId, docId, toRelationship);
            }
        }

        current = parentId;
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

    std::string memo{"one-time payment on proposal: " + to_str(proposal.getID())};

    auto tokens = AssetBatch {
        .reward = m_daoSettings->getSettingOrDefault<asset>(common::REWARD_TOKEN),
        .peg = m_daoSettings->getSettingOrDefault<asset>(common::PEG_TOKEN),
        .voice = m_daoSettings->getOrFail<asset>(common::VOICE_TOKEN)
    };

    auto detailsGroup = contentWrapper.getGroupOrFail(DETAILS);

    auto payoutItems = std::vector<std::string>{common::VOICE_AMOUNT};
    
    //DAO could not have defined peg or reward token
    if (tokens.peg.is_valid()) payoutItems.push_back(common::PEG_AMOUNT);
    if (tokens.reward.is_valid()) payoutItems.push_back(common::REWARD_AMOUNT);

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