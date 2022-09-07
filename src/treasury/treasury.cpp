#include "treasury/treasury.hpp"
#include "treasury/common.hpp"
#include "settings.hpp"

namespace hypha::treasury {

using namespace common;

Treasury::Treasury(dao& dao, uint64_t id)
    : TypedDocument(dao, id, types::TREASURY)
{}

Treasury::Treasury(dao& dao, uint64_t daoID, TreasuryData data)
    : TypedDocument(dao, types::TREASURY)
{

    auto cgs = ContentGroups {
        ContentGroup { //Details groups
            { CONTENT_GROUP_LABEL, DETAILS },
            data.dao.extract(),
        }
    };
    
    initializeDocument(dao, cgs);

    auto settings = Settings(dao, getId());

    //Default settings
    settings.setSettings(SETTINGS, {
        { fields::THRESHOLD, 1 },
        { fields::REDEMPTIONS_ENABLED, true }
    });
}

uint64_t Treasury::getDAO() {
    return getContentWrapper().getOrFail()
}

}