#pragma once

#include <optional>

#include <typed_document.hpp>

#include <macros.hpp>

#include <member.hpp>

namespace hypha::upvote_election
{

class UpvoteElection;

class ElectionGroup : public TypedDocument
{
    DECLARE_DOCUMENT(
        Data,
        PROPERTY(type, std::string, Type, USE_GET),
        PROPERTY(member_count, int64_t, MemberCount, USE_GET),
        PROPERTY(group_id, int64_t, RoundId, USE_GET)
    )
public:
    ElectionGroup(dao& dao, uint64_t id);
    ElectionGroup(dao& dao, uint64_t group_id, std::vector<uint64_t> member_ids, Data data);

    std::optional<Member> getWinner();

    // UpvoteElection getElection();
    // std::vector<uint64_t> getWinners();
    // int64_t getAccountPower(uint64_t accountId);
    // bool isCandidate(uint64_t accountId);
    // void addCandidate(uint64_t accountId);
    
private:
    virtual const std::string buildNodeLabel(ContentGroups &content) override
    {
        return "Election Group";
    }
};

using ElectionGroupData = ElectionGroup::Data;

} // namespace hypha::upvote_election