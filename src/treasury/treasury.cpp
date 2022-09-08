#include "treasury/treasury.hpp"
#include "treasury/common.hpp"
#include "settings.hpp"

namespace hypha::treasury {

using namespace common;

Treasury::Treasury(dao& dao, uint64_t id)
    : TypedDocument(dao, id, types::TREASURY)
{}

Treasury::Treasury(dao& dao, Data data)
    : TypedDocument(dao, types::TREASURY)
{   
    auto daoID = data.dao;
    auto cgs = convert(std::move(data));

    initializeDocument(dao, cgs);

    // auto settings = Settings(dao, getId());

    // //Default settings
    // settings.setSettings(SETTINGS, {
    //     { fields::THRESHOLD, 1 },
    //     { fields::REDEMPTIONS_ENABLED, true }
    // });

    //Initialize edges to/from DAO

}

}