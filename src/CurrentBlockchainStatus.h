//
// Created by mwo on 14/12/16.
//

#ifndef RESTBED_XMR_CURRENTBLOCKCHAINSTATUS_H
#define RESTBED_XMR_CURRENTBLOCKCHAINSTATUS_H

#define MYSQLPP_SSQLS_NO_STATICS 1

#include "MicroCore.h"
#include "ssqlses.h"
#include "BlockchainSetup.h"
#include "TxSearch.h"

#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>


namespace xmreg {

using namespace std;

class XmrAccount;
class MySqlAccounts;

static mutex searching_threads_map_mtx;
static mutex getting_mempool_txs;

/*
* This is a thread class. Probably it should be singleton, as we want
* only onstance of this class, but instead I will just make one instance of
* it in main.cpp and will be injecting it around. Copies are disabled, so
* we should not make a copy by accident. Moves are enabled though.
*
* This way its much easier to mock it for unit testing.
*/
class CurrentBlockchainStatus : public std::enable_shared_from_this<CurrentBlockchainStatus>
{
public:
    // vector of mempool transactions that all threads
    // can refer to
    //                               recieved_time, tx
    using mempool_txs_t = vector<pair<uint64_t, transaction>>;


    //                            tx_hash      , tx,          height , timestamp, is_coinbase
    using txs_tuple_t = std::tuple<crypto::hash, transaction, uint64_t, uint64_t, bool>;

    atomic<uint64_t> current_height;

    bool is_running;

    CurrentBlockchainStatus(BlockchainSetup _bc_setup);

    virtual void
    start_monitor_blockchain_thread();

    virtual uint64_t
    get_current_blockchain_height();

    virtual void
    update_current_blockchain_height();

    virtual bool
    init_monero_blockchain();

    virtual bool
    is_tx_unlocked(uint64_t unlock_time, uint64_t block_height);

    virtual bool
    is_tx_spendtime_unlocked(uint64_t unlock_time, uint64_t block_height);

    virtual bool
    get_block(uint64_t height, block &blk);

    virtual vector<block>
    get_blocks_range(uint64_t const& h1, uint64_t const& h2);

    virtual bool
    get_block_txs(const block &blk,
                  vector<transaction> &blk_txs,
                  vector<crypto::hash>& missed_txs);

    virtual bool
    get_txs(vector<crypto::hash> const& txs_to_get,
            vector<transaction>& txs,
            vector<crypto::hash>& missed_txs);

    virtual bool
    tx_exist(const crypto::hash& tx_hash);

    virtual bool
    tx_exist(const crypto::hash& tx_hash, uint64_t& tx_index);

    virtual bool
    tx_exist(const string& tx_hash_str, uint64_t& tx_index);

    virtual bool
    tx_exist(const string& tx_hash_str);

    virtual bool
    get_tx_with_output(uint64_t output_idx, uint64_t amount,
                       transaction& tx, uint64_t& output_idx_in_tx);

    virtual bool
    get_output_keys(const uint64_t& amount,
                    const vector<uint64_t>& absolute_offsets,
                    vector<cryptonote::output_data_t>& outputs);

    virtual string
    get_account_integrated_address_as_str(crypto::hash8 const& payment_id8);

    virtual string
    get_account_integrated_address_as_str(string const& payment_id8_str);

    virtual bool
    get_output(const uint64_t amount,
               const uint64_t global_output_index,
               COMMAND_RPC_GET_OUTPUTS_BIN::outkey& output_info);

    virtual bool
    get_amount_specific_indices(const crypto::hash& tx_hash,
                                vector<uint64_t>& out_indices);

    virtual bool
    get_random_outputs(const vector<uint64_t>& amounts,
                       const uint64_t& outs_count,
                       vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount>& found_outputs);

    virtual bool
    get_dynamic_per_kb_fee_estimate(uint64_t& fee_estimated);

    virtual bool
    commit_tx(const string& tx_blob, string& error_msg,
              bool do_not_relay = false);

    virtual bool
    read_mempool();

    virtual vector<pair<uint64_t, transaction>>
    get_mempool_txs();

    virtual bool
    search_if_payment_made(
            const string& payment_id_str,
            const uint64_t& desired_amount,
            string& tx_hash_with_payment);


    virtual string
    get_payment_id_as_string(const transaction& tx);

    virtual output_data_t
    get_output_key(uint64_t amount,
                   uint64_t global_amount_index);

    // definitions of these function are at the end of this file
    // due to forward declaraions of TxSearch
    virtual bool
    start_tx_search_thread(XmrAccount acc);

    virtual bool
    ping_search_thread(const string& address);

    virtual bool
    search_thread_exist(const string& address);

    virtual bool
    get_xmr_address_viewkey(const string& address_str,
                            address_parse_info& address,
                            secret_key& viewkey);
    virtual bool
    find_txs_in_mempool(const string& address_str,
                        json& transactions);

    virtual bool
    find_tx_in_mempool(crypto::hash const& tx_hash,
                       transaction& tx);

    virtual bool
    find_key_images_in_mempool(std::vector<txin_v> const& vin);

    virtual bool
    find_key_images_in_mempool(transaction const& tx);

    virtual bool
    get_tx(crypto::hash const& tx_hash, transaction& tx);

    virtual bool
    get_tx(string const& tx_hash_str, transaction& tx);

    virtual bool
    get_tx_block_height(crypto::hash const& tx_hash, int64_t& tx_height);

    virtual bool
    set_new_searched_blk_no(const string& address,
                            uint64_t new_value);

    virtual bool
    get_searched_blk_no(const string& address,
                        uint64_t& searched_blk_no);

    virtual bool
    get_known_outputs_keys(string const& address,
                           unordered_map<public_key, uint64_t>& known_outputs_keys);

    virtual void
    clean_search_thread_map();

    /*
     * The frontend requires rct field to work
     * the filed consisitct of rct_pk, mask, and amount.
     */
    virtual tuple<string, string, string>
    construct_output_rct_field(
            const uint64_t global_amount_index,
            const uint64_t out_amount);


    inline virtual BlockchainSetup const&
    get_bc_setup() const
    {
        return bc_setup;
    }



    virtual bool
    get_txs_in_blocks(vector<block> const& blocks,
                      vector<txs_tuple_t>& txs_data);

    // default destructor is fine
    virtual ~CurrentBlockchainStatus() = default;

    // we dont want any copies of the objects of this class
    CurrentBlockchainStatus(CurrentBlockchainStatus const&) = delete;
    CurrentBlockchainStatus& operator= (CurrentBlockchainStatus const&) = delete;

    // but we want moves, default ones should be fine
    CurrentBlockchainStatus(CurrentBlockchainStatus&&) = default;
    CurrentBlockchainStatus& operator= (CurrentBlockchainStatus&&) = default;

protected:

    // parameters used to connect/read monero blockchain
    BlockchainSetup bc_setup;

   // since this class monitors current status
    // of the blockchain, its seems logical to
    // make object for accessing the blockchain here
    unique_ptr<xmreg::MicroCore> mcore;
    cryptonote::Blockchain *core_storage;

    // vector of mempool transactions that all threads
    // can refer to
    //           <recieved_time, transaction>
    mempool_txs_t mempool_txs;

    // map that will keep track of search threads. In the
    // map, key is address to which a running thread belongs to.
    // make it static to guarantee only one such map exist.
    map<string, unique_ptr<TxSearch>> searching_threads;

    // thread that will be dispachaed and will keep monitoring blockchain
    // and mempool changes
    std::thread m_thread;
};


}
#endif //RESTBED_XMR_CURRENTBLOCKCHAINSTATUS_H
