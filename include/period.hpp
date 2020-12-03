#pragma once
#include <eosio/name.hpp>
#include <eosio/crypto.hpp>

#include <document_graph/document.hpp>

#include <dao.hpp>

namespace hypha
{
    class Period
    {
    public:
        Period(dao &dao,
               const eosio::time_point &start_time,
               const eosio::time_point &end_time,
               const std::string &label);

        void emplace();

        dao &m_dao;
        const eosio::time_point &start_time;
        const eosio::time_point &end_time;
        const std::string &label;
        Document m_document;
    };
} // namespace hypha