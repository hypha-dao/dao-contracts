#pragma once
#include <typed_document.hpp>
#include <string>
#include <eosio/name.hpp>

// Reaction edges
// Member1 Liked Proposal1 and Loved Proposal2
// Member2 Loved Proposal3
//
//                                    ┌───────reaction─┐
//                                    ▼                ▼
//                  ┌reactionlink─►LIKED-P1    ┌───►Proposal1
//                  ▼                          │
//         ┌─►Member1◄────────reactedto────────┤
//         │                                   │
//         │                                   └───►Proposal2◄─┐
//         │                                                   │
//         │  Member2◄────────reactedto──────────────┐         │
//         │      ▲                                  ▼         │
//         │      reactionlink►LOVED-P3◄──────┐     Proposal3  │
//         │                                  │         ▲      │
//         └──reactionlink─┐                 reaction───┘      │
//                         └────►LOVED-P2◄─────┐               │
//                                             └─reaction──────┘
namespace hypha
{
    class dao;
    class Likeable;

    class Reaction: public TypedDocument
    {
        public:
            Reaction(dao& dao, uint64_t id);
            Reaction(
                dao&dao,
                Likeable& likeable,
                const eosio::name reaction
            );

            void remove();
            void react(eosio::name who);
            void unreact(eosio::name who);

            static Reaction getReaction(dao& dao, Likeable& likeable, const eosio::name reaction);
            static Reaction getReactionByUser(dao& dao, Likeable& likeable, const eosio::name who);

        protected:
            virtual const std::string buildNodeLabel(ContentGroups &content);
            uint64_t getLikeableId();
    };
}
