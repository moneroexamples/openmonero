#include "TxUnlockChecker.h"

namespace xmreg
{

uint64_t
TxUnlockChecker::get_current_time() const
{
    return static_cast<uint64_t>(time(NULL));
}


uint64_t
TxUnlockChecker::get_v2height(network_type net_type) const
{
    return net_type == TESTNET ?
                624634 : net_type == STAGENET ?
                   (uint64_t)-1 : 1009827;
}

uint64_t
TxUnlockChecker::get_leeway(
        uint64_t tx_block_height,
        network_type net_type) const
{
    return tx_block_height < get_v2height(net_type)
            ? CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS_V1
            : CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS_V2;
}

bool
TxUnlockChecker::is_unlocked(
        network_type net_type,
        uint64_t current_blockchain_height,
        uint64_t tx_unlock_time,
        uint64_t tx_block_height) const
{
    if(tx_unlock_time < CRYPTONOTE_MAX_BLOCK_NUMBER)
    {
        //interpret as block index

        if(current_blockchain_height
                + CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS
                >= tx_unlock_time)
            return true;
        else
            return false;
    }
    else
    {
        //interpret unlock_time as timestamp

        if(get_current_time() + get_leeway(tx_block_height, net_type)
                >= tx_unlock_time)
            return true;
        else
            return false;
    }

    // this part will never execute. dont need it
//    if(tx_block_height + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE
//            > current_blockchain_height + 1)
//        return false;

     return true;
}



}
