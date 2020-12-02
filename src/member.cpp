#include <member.hpp>
#include <common.hpp>
#include <document_graph/document_graph.hpp>
#include <document_graph/content.hpp>
#include <document_graph/edge.hpp>
#include <document_graph/util.hpp>

namespace hypha
{
    Member::Member (eosio::name member) : m_member{member} {}

    const eosio::checksum256 Member::getHash (const eosio::name &member) 
    {
        ContentGroups contentGroups = Document::rollup (Content (common::MEMBER_STRING, member));
        return Document::hashContents(contentGroups);
    }

    const bool Member::isMember (const eosio::name &rootNode, const eosio::name &member)
    {
        // create hash to represent this member account
        auto memberHash = Member::getHash(member);
        ContentGroups contentGroups = Document::rollup (Content (common::ROOT_NODE, rootNode));
        auto rootNodeHash = Document::hashContents(contentGroups);

        return Edge::exists (rootNode, rootNodeHash, memberHash, common::MEMBER);
    }

    Document Member::getOrNew (const eosio::name& contract, const eosio::name &creator, const eosio::name &member)
    {
        return Document::getOrNew(contract, creator, common::MEMBER_STRING, member);
    }
}