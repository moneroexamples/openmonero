//
// Created by mwo on 14/12/16.
//

#ifndef RESTBED_XMR_CURRENTBLOCKCHAINSTATUS_H
#define RESTBED_XMR_CURRENTBLOCKCHAINSTATUS_H

#define MYSQLPP_SSQLS_NO_STATICS 1

#include "MicroCore.h"
#include "ssqlses.h"

#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>


namespace xmreg {

using namespace std;

class TxSearch;
class XmrAccount;
class MySqlAccounts;

static mutex searching_threads_map_mtx;
static mutex getting_mempool_txs;

/*
* This is a thread class
*/
struct CurrentBlockchainStatus
{

    // vector of mempool transactions that all threads
    // can refer to
    //           <recieved_time, transaction>
    using mempool_txs_t = vector<pair<uint64_t, transaction>>;

    static string blockchain_path;

    static atomic<uint64_t> current_height;

    static string deamon_url;

    static network_type net_type;

    static bool do_not_relay;

    static std::thread m_thread;

    static bool is_running;

    static uint64_t refresh_block_status_every_seconds;

    static uint64_t max_number_of_blocks_to_import;

    static uint64_t search_thread_life_in_seconds;

    static string   import_payment_address_str;
    static string   import_payment_viewkey_str;
    static uint64_t import_fee;
    static uint64_t spendable_age;
    static uint64_t spendable_age_coinbase;

    static address_parse_info import_payment_address;
    static secret_key         import_payment_viewkey;

    // vector of mempool transactions that all threads
    // can refer to
    //           <recieved_time, transaction>
    static mempool_txs_t mempool_txs;

    // map that will keep track of search threads. In the
    // map, key is address to which a running thread belongs to.
    // make it static to guarantee only one such map exist.
    static map<string, unique_ptr<TxSearch>> searching_threads;

    // since this class monitors current status
    // of the blockchain, its seems logical to
    // make object for accessing the blockchain here
    static unique_ptr<xmreg::MicroCore> mcore;
    static cryptonote::Blockchain *core_storage;

    static
    void start_monitor_blockchain_thread();

    static uint64_t
    get_current_blockchain_height();

    static void
    update_current_blockchain_height();

    static bool
    init_monero_blockchain();

    static bool
    is_tx_unlocked(uint64_t unlock_time, uint64_t block_height);

    static bool
    is_tx_spendtime_unlocked(uint64_t unlock_time, uint64_t block_height);

    static bool
    get_block(uint64_t height, block &blk);

    static bool
    get_block_txs(const block &blk, list <transaction> &blk_txs);

    static bool
    tx_exist(const crypto::hash& tx_hash);

    static bool
    tx_exist(const crypto::hash& tx_hash, uint64_t& tx_index);

    static bool
    tx_exist(const string& tx_hash_str, uint64_t& tx_index);

    static bool
    tx_exist(const string& tx_hash_str);

    static bool
    get_tx_with_output(uint64_t output_idx, uint64_t amount,
                       transaction& tx, uint64_t& output_idx_in_tx);

    static bool
    get_output_keys(const uint64_t& amount,
                    const vector<uint64_t>& absolute_offsets,
                    vector<cryptonote::output_data_t>& outputs);

    static string
    get_account_integrated_address_as_str(crypto::hash8 const& payment_id8);

    static string
    get_account_integrated_address_as_str(string const& payment_id8_str);

    static bool
    get_output(const uint64_t amount,
               const uint64_t global_output_index,
               COMMAND_RPC_GET_OUTPUTS_BIN::outkey& output_info);

    static bool
    get_amount_specific_indices(const crypto::hash& tx_hash,
                                vector<uint64_t>& out_indices);

    static bool
    get_random_outputs(const vector<uint64_t>& amounts,
                       const uint64_t& outs_count,
                       vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount>& found_outputs);

    static bool
    get_dynamic_per_kb_fee_estimate(uint64_t& fee_estimated);

    static bool
    commit_tx(const string& tx_blob, string& error_msg,  bool do_not_relay = false);

    static bool
    read_mempool();

    static vector<pair<uint64_t, transaction>>
    get_mempool_txs();

    static bool
    search_if_payment_made(
            const string& payment_id_str,
            const uint64_t& desired_amount,
            string& tx_hash_with_payment);


    static string
    get_payment_id_as_string(const transaction& tx);

    static output_data_t
    get_output_key(uint64_t amount,
                   uint64_t global_amount_index);

    // definitions of these function are at the end of this file
    // due to forward declaraions of TxSearch
    static bool
    start_tx_search_thread(XmrAccount acc);

    static bool
    ping_search_thread(const string& address);

    static bool
    search_thread_exist(const string& address);

    static bool
    get_xmr_address_viewkey(const string& address_str,
                            address_parse_info& address,
                            secret_key& viewkey);
    static bool
    find_txs_in_mempool(const string& address_str,
                        json& transactions);

    static bool
    find_tx_in_mempool(crypto::hash const& tx_hash,
                       transaction& tx);

    static bool
    get_tx(crypto::hash const& tx_hash, transaction& tx);

    static bool
    get_tx(string const& tx_hash_str, transaction& tx);

    static bool
    get_tx_block_height(crypto::hash const& tx_hash, int64_t& tx_height);

    static bool
    set_new_searched_blk_no(const string& address,
                            uint64_t new_value);

    static bool
    get_searched_blk_no(const string& address,
                        uint64_t& searched_blk_no);

    static bool
    get_known_outputs_keys(string const& address,
                           vector<pair<string, uint64_t>>& known_outputs_keys);

    static void
    clean_search_thread_map();

    /*
     * The frontend requires rct field to work
     * the filed consisitct of rct_pk, mask, and amount.
     */
    static tuple<string, string, string>
    construct_output_rct_field(
            const uint64_t global_amount_index,
            const uint64_t out_amount);

};


}
#endif //RESTBED_XMR_CURRENTBLOCKCHAINSTATUS_H
