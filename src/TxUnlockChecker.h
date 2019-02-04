#ifndef TXUNLOCKCHECKER_H
#define TXUNLOCKCHECKER_H

#include "src/monero_headers.h"

namespace xmreg
{

using namespace cryptonote;
using namespace crypto;
using namespace std;


// class based on
// bool wallet2::is_tx_spendtime_unlocked(uint64_t unlock_time, uint64_t block_height) const
// and
// bool wallet2::is_transfer_unlocked(uint64_t unlock_time, uint64_t block_height) const
// from monero.
// we make it separate class as its easier to mock it
// later on in our tests
class TxUnlockChecker
{
public:
    TxUnlockChecker() = default;

    virtual uint64_t
    get_current_time() const;

    virtual uint64_t
    get_v2height(network_type net_type) const;

    virtual uint64_t
    get_leeway(uint64_t tx_block_height, network_type net_type) const;

    virtual bool
    is_unlocked(network_type net_type,
                uint64_t current_blockchain_height,
                uint64_t tx_unlock_time,
                uint64_t tx_block_height) const;

    virtual ~TxUnlockChecker() = default;

};

}

#endif // TXUNLOCKCHECKER_H
