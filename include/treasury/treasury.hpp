#pragma once

#include <typed_document.hpp>
#include <macros.hpp>

#ifdef USE_TREASURY

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
    /**
    * @brief Data that we store in the document
    * @dgnodedata
    */
    DECLARE_DOCUMENT(
        Data, 
        PROPERTY(dao, int64_t, DaoID, USE_GETSET)
    );

public:
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
     * @param data 
     */
    Treasury(dao& dao, Data data);

    virtual const std::string buildNodeLabel(ContentGroups &content) override
    {
        return "Treasury";
    }

    void removeTreasurer(uint64_t memberID);

    void addTreasurer(uint64_t memberID);

    void checkTreasurerAuth();

    static Treasury getFromDaoID(dao& dao, uint64_t daoID);
};

using TreasuryData = Treasury::Data;

}

#endif