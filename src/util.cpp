#include <util.hpp>
#include <eosio/crypto.hpp>
#include <document_graph/document.hpp>
#include <document_graph/content_wrapper.hpp>
#include <common.hpp>

namespace hypha {

    eosio::checksum256 getRoot (const eosio::name& contract)
    {
        ContentGroups cgs = Document::rollup(Content(common::ROOT_NODE, contract));
        return Document::hashContents(cgs);
    }
}