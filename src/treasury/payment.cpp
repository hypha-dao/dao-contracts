#include "treasury/payment.hpp"
#include "treasury/common.hpp"

namespace hypha::treasury {

using namespace common;

TrsyPayment::TrsyPayment(dao& dao, uint64_t id)
    : TypedDocument(dao, id, types::TREASURY_PAYMENT)
{}

TrsyPayment::TrsyPayment(dao& dao, uint64_t treasuryID, uint64_t redemptionID, Data data)
    : TypedDocument(dao, types::TREASURY_PAYMENT)
{
    auto cgs = convert(std::move(data));

    initializeDocument(dao, cgs);
}

} // namespace treasury
