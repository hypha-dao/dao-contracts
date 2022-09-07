#pragma once

#include <typed_document.hpp>
#include <util.hpp>

namespace hypha::treasury {

/**
 * @brief Treasury document representation.
 * Each DAO has an associated Treasury document which is referenced by other related 
 * documents like 'Treasurers', 'Redemptions', 'Treasury Payments', etc.
 * @dgnode Treasury
 * @connection DAO -> treasury -> Treasury
 * @connection Treasury -> treasuryof -> Dao
 * @connection Treasury -> settings -> Settings
 * @connection Treasury -> treasurer -> Member
 * @connection Member -> treasurerof -> Treasury
 * @connection Treasury -> redemption -> Redemption
 * @connection Treasury -> payment -> TrsyPayment
 */

class Treasury : public TypedDocument
{
public:
    /**
    * @brief Data that we store in the document
    * @dgnodedata
    */
    struct TreasuryData {
        template<class T>
        using TypedContent = hypha::util::TypedContent<T>;

        TypedContent<int64_t> dao; //Dao
    };

    /**
     * @brief Construct a Treasury object from an existing treasury document
     * 
     * @param dao 
     * @param id 
     */
    Treasury(dao& dao, uint64_t id);

    /**
     * @brief Construct a new Treasury document in the Document Graph
     * 
     * @param dao 
     * @param daoID 
     * @param data 
     */
    Treasury(dao& dao, uint64_t daoID, TreasuryData data);

    uint64_t getDAO();
};

}