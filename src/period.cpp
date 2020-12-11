#include <period.hpp>
#include <common.hpp>
#include <util.hpp>
#include <document_graph/edge.hpp>

namespace hypha
{
    Period::Period(dao &dao, const eosio::time_point &start_time, 
                   const eosio::time_point &end_time, const std::string &label) 
        : m_dao{dao}, start_time{start_time}, end_time{end_time}, label{label}
    {
        // Content id {};
        
        Content startDate (START_TIME, start_time);
        Content endDate (END_TIME, end_time);
        Content labelContent (LABEL, label);

        ContentGroup cg {};
        // cg.push_back(id);
        cg.push_back(startDate);
        cg.push_back(endDate);
        cg.push_back(labelContent);

        ContentGroups cgs {};
        cgs.push_back(cg);

        m_document = Document (m_dao.get_self(), m_dao.get_self(), cgs);

        // TODO: Should we create a document or just a table entry?        
    }


    void Period::emplace () 
    {
        // TODO: We should have some input cleansing to make sure this period is mutually exclusive
        // to other periods.  Also, they should be sequential and we rely on the oracle to enforce this. 

        // TODO: make this multi-member, it may not be "root"
        eosio::checksum256 root = getRoot(m_dao.get_self());

        Edge::write (m_dao.get_self(), m_dao.get_self(), root, m_document.getHash(), common::PERIOD);

        dao::PeriodTable period_t(m_dao.get_self(), m_dao.get_self().value);
		period_t.emplace(m_dao.get_self(), [&](auto &p) {
			p.id = period_t.available_primary_key();
            p.start_time = start_time;
            p.end_time = end_time;
            p.label = label;
		});
    }

}