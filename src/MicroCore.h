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

    tx_memory_pool m_mempool;
    Blockchain core_storage;

    hw::device* m_device;

    network_type nettype;

    bool initialization_succeded {false};
    
public:

    //   <amoumt,
    //    tuple<total_instances, unlocked_instances, recent_instances>
    using histogram_map = std::map<uint64_t,
                               std::tuple<uint64_t,  uint64_t, uint64_t>>;

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

    virtual void
    get_output_key(uint64_t amount,
                   vector<uint64_t> const& absolute_offsets,
                   vector<cryptonote::output_data_t>& outputs)
    {
        core_storage.get_db()
                .get_output_key(epee::span<const uint64_t>(&amount, 1),
                                absolute_offsets, outputs);
    }

    virtual output_data_t
    get_output_key(uint64_t amount,
                   uint64_t global_amount_index)
    {
        return core_storage.get_db()
                    .get_output_key(amount, global_amount_index);
    }

    virtual bool
    get_transactions(
            const std::vector<crypto::hash>& txs_ids,
            std::vector<transaction>& txs,
            std::vector<crypto::hash>& missed_txs) const
    {
        return core_storage.get_transactions(txs_ids, txs, missed_txs);
    }

    virtual std::vector<block>
    get_blocks_range(const uint64_t& h1, const uint64_t& h2) const
    {
        return core_storage.get_db().get_blocks_range(h1, h2);
    }
    
    //virtual std::vector<block>
    //get_blocks_range(const uint64_t& h1, const uint64_t& h2) const
    //{
        //std::vector<std::pair<cryptonote::blobdata,block>> blobblocks;
        //std::vector<blobdata> txs;
        //vector<block> blocks;

        //{
            //std::lock_guard<std::mutex> lock(mtx1);
            //cout << "tid: " << std::this_thread::get_id() << endl;;
            //core_storage.get_blocks(h1, h2-h1+1, blobblocks, txs);
        //}

        //blocks.reserve(blobblocks.size());

        //for (auto const& bpair: blobblocks)
            //blocks.push_back(std::move(bpair.second));

        //return blocks; 
    //}

    virtual uint64_t
    get_tx_unlock_time(crypto::hash const& tx_hash) const
    {
        return core_storage.get_db().get_tx_unlock_time(tx_hash);
    }

    virtual bool
    have_tx(crypto::hash const& tx_hash) const
    {
        return core_storage.have_tx(tx_hash);
    }

    virtual bool
    tx_exists(crypto::hash const& tx_hash, uint64_t& tx_id) const
    {
        return core_storage.get_db().tx_exists(tx_hash, tx_id);
    }

    virtual tx_out_index
    get_output_tx_and_index(uint64_t const& amount, uint64_t const& index) const
    {
        return core_storage.get_db().get_output_tx_and_index(amount, index);
    }

    virtual uint64_t
    get_tx_block_height(crypto::hash const& tx_hash) const
    {
        return core_storage.get_db().get_tx_block_height(tx_hash);
    }

    virtual std::vector<uint64_t>
    get_tx_amount_output_indices(uint64_t const& tx_id) const
    {
        return core_storage.get_db()
                .get_tx_amount_output_indices(tx_id).front();
    }

    virtual bool
    get_mempool_txs(
            std::vector<tx_info>& tx_infos,
            std::vector<spent_key_image_info>& key_image_infos) const
    {
        return m_mempool.get_transactions_and_spent_keys_info(
                    tx_infos, key_image_infos);
    }

    virtual uint64_t
    get_current_blockchain_height() const
    {
        return core_storage.get_current_blockchain_height();
    }

    virtual void
    get_output_tx_and_index(
            const uint64_t& amount,
            const std::vector<uint64_t>& offsets,
            std::vector<tx_out_index>& indices) const
    {
        //                           tx_hash     , index in tx
        // tx_out_index is std::pair<crypto::hash, uint64_t>;
        core_storage.get_db().get_output_tx_and_index(
                    amount, offsets, indices);
    }

    virtual bool
    get_output_histogram(
            vector<uint64_t> const& amounts,
            uint64_t min_count,
            histogram_map& histogram,
            bool unlocked = true,
            uint64_t recent_cutoff = 0) const;


    // mimicks core_rpc_server::on_get_output_histogram(..)
    virtual bool
    get_output_histogram(
            COMMAND_RPC_GET_OUTPUT_HISTOGRAM::request const& req,
            COMMAND_RPC_GET_OUTPUT_HISTOGRAM::response& res) const;


    virtual bool
    get_outs(COMMAND_RPC_GET_OUTPUTS_BIN::request const& req,
             COMMAND_RPC_GET_OUTPUTS_BIN::response& res) const
    {
        return core_storage.get_outs(req, res);
    }

    virtual uint64_t
    get_dynamic_base_fee_estimate(uint64_t const& grace_blocks) const
    {
        return core_storage.get_dynamic_base_fee_estimate(grace_blocks);
    }

    bool
    get_block_complete_entry(block const& b, block_complete_entry& bce);

    virtual bool
    get_block_from_height(uint64_t height, block& blk) const;

    virtual bool
    get_tx(crypto::hash const& tx_hash, transaction& tx) const;

    virtual bool
    decrypt_payment_id(crypto::hash8& payment_id,
                       public_key const& public_key,
                       secret_key const& secret_key)
    {
        return m_device->decrypt_payment_id(payment_id,
                                            public_key,
                                            secret_key);
    }

    virtual bool
    init_success() const;    

    virtual ~MicroCore() = default;
};


}


#endif //XMREG01_MICROCORE_H
