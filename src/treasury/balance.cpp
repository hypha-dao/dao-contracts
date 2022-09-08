#include "treasury/balance.hpp"
#include "treasury/common.hpp"

#include <common.hpp>
#include <dao.hpp>
#include <document_graph/edge.hpp>

namespace hypha::treasury {

using namespace common;

Balance::Balance(dao& dao, uint64_t id)
    : TypedDocument(dao, id, types::BALANCE)
{}

Balance::Balance(dao& dao, uint64_t owner, Data data)
    : TypedDocument(dao, types::BALANCE)
{
    auto cgs = convert(std::move(data));

    initializeDocument(dao, cgs);
}

Balance Balance::getOrCreate(dao& dao, uint64_t daoID, uint64_t owner)
{
    auto balanceEdges = dao.getGraph().getEdgesFrom(owner, links::REDEEM_BALANCE);

    //Check if the member has an existing balance document for the required DAO
    for (auto& edge : balanceEdges) {
        Balance balance(dao, edge.getToNode());
        if (balance.getDaoID() == daoID) {
            return balance;
        }
    }

    auto daoSettings = dao.getSettingsDocument(daoID);

    auto cashToken = daoSettings->getOrFail<asset>(hypha::common::PEG_TOKEN);

    //Start with a 0 balance
    cashToken.amount = 0;
    //Create a new one if not
    return Balance(dao, owner, Data {
        .quantity = cashToken,
        .dao = static_cast<int64_t>(daoID)
    });
}

} // namespace treasury
