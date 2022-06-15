#pragma once
#include <typed_document.hpp>
#include <string>
#include <eosio/name.hpp>

namespace hypha
{
    class dao;

    using std::string;
    using eosio::asset;

    class Circle : public TypedDocument
    {
    public:
        Circle(dao& dao, uint64_t id);
        Circle(
            dao& dao,
            const string title,
            const string description,
            const uint64_t dao_id,
            const uint64_t parent_circle,
            const asset budget
        );

        void setBudget(asset new_budget);
        void addMember(uint64_t member_id);
        void addContribution(uint64_t contribution_id);

        void votingPower(uint64_t member_id, uint64_t contribution_id);

   protected:
           virtual const std::string buildNodeLabel(ContentGroups &content);
    };
}
