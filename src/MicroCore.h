//
// Created by mwo on 5/11/15.
//

#ifndef XMREG01_MICROCORE_H
#define XMREG01_MICROCORE_H

#include <iostream>
#include <random>

#include "monero_headers.h"
#include "tools.h"

namespace xmreg
{
using namespace cryptonote;
using namespace crypto;
using namespace std;

/**
 * Micro version of cryptonode::core class
 * Micro version of constructor,
 * init and destructor are implemted.
 *
 * Just enough to read the blockchain
 * database for use in the example.
 */
class MicroCore {

    string blockchain_path;

    Blockchain core_storage;
    tx_memory_pool m_mempool;

    hw::device* m_device;

    network_type nettype;

public:
    MicroCore();

    /**
     * Initialized the MicroCore object.
     *
     * Create BlockchainLMDB on the heap.
     * Open database files located in blockchain_path.
     * Initialize m_blockchain_storage with the BlockchainLMDB object.
     */    
    virtual bool
    init(const string& _blockchain_path, network_type nt);

    virtual Blockchain const&
    get_core() const;

    virtual tx_memory_pool const&
    get_mempool() const;

    virtual hw::device* const
    get_device() const;

    template<typename... T>
    auto get_output_key(T&&... args)
    {
        return core_storage.get_db().get_output_key(std::forward<T>(args)...);
    }

    template <typename... T>
    auto get_transactions(T&&... args) const
    {
        return core_storage.get_transactions(std::forward<T>(args)...);
    }

    template <typename... T>
    auto get_blocks_range(T&&... args) const
    {
        return core_storage.get_db().get_blocks_range(std::forward<T>(args)...);
    }

    template <typename... T>
    auto have_tx(T&&... args) const
    {
        return core_storage.have_tx(std::forward<T>(args)...);
    }

    template<typename... T>
    auto tx_exists(T&&... args) const
    {
        return core_storage.get_db().tx_exists(std::forward<T>(args)...);
    }

    template<typename... T>
    auto get_output_tx_and_index(T&&... args) const
    {
        return core_storage.get_db().get_output_tx_and_index(std::forward<T>(args)...);
    }

    template<typename... T>
    auto get_tx_block_height(T&&... args) const
    {
        return core_storage.get_db().get_tx_block_height(std::forward<T>(args)...);
    }

    template<typename... T>
    auto get_tx_amount_output_indices(T&&... args) const
    {
        return core_storage.get_db().get_tx_amount_output_indices(std::forward<T>(args)...);
    }

    template<typename... T>
    auto get_mempool_txs(T&&... args) const
    {
        return m_mempool.get_transactions_and_spent_keys_info(std::forward<T>(args)...);
    }

    virtual uint64_t
    get_current_blockchain_height() const
    {
        return core_storage.get_current_blockchain_height();
    }

    virtual bool
    get_block_from_height(uint64_t height, block& blk) const;

    virtual bool
    get_tx(crypto::hash const& tx_hash, transaction& tx) const;
};

}



#endif //XMREG01_MICROCORE_H
