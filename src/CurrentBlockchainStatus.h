//
// Created by mwo on 14/12/16.
//

#ifndef RESTBED_XMR_CURRENTBLOCKCHAINSTATUS_H
#define RESTBED_XMR_CURRENTBLOCKCHAINSTATUS_H


#include "MicroCore.h"
//#include "TxSearch.h"

#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>


namespace xmreg {


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
    static string blockchain_path;

    static atomic<uint64_t> current_height;

    static string deamon_url;

    static bool testnet;

    static std::thread m_thread;

    static bool is_running;

    static uint64_t refresh_block_status_every_seconds;

    // map that will keep track of search threads. In the
    // map, key is address to which a running thread belongs to.
    // make it static to guarantee only one such map exist.
    static map<string, shared_ptr<TxSearch>> searching_threads;

    static string   import_payment_address;
    static string   import_payment_viewkey;
    static uint64_t import_fee;

    static account_public_address address;
    static secret_key             viewkey;

    // vector of mempool transactions that all threads
    // can refer to
    static vector<transaction> mempool_txs;

    // since this class monitors current status
    // of the blockchain, its seems logical to
    // make object for accessing the blockchain here
    static xmreg::MicroCore mcore;
    static cryptonote::Blockchain *core_storage;

    static
    void start_monitor_blockchain_thread();

    static
    uint64_t
    get_current_blockchain_height();

    static void
    set_blockchain_path(const string &path);

    static void
    set_testnet(bool is_testnet);

    static bool
    init_monero_blockchain();

    static bool
    get_block(uint64_t height, block &blk);

    static bool
    get_block_txs(const block &blk, list <transaction> &blk_txs);

    static bool
    get_output_keys(const uint64_t& amount,
                    const vector<uint64_t>& absolute_offsets,
                    vector<cryptonote::output_data_t>& outputs);

    static bool
    get_amount_specific_indices(const crypto::hash& tx_hash,
                                vector<uint64_t>& out_indices);

    static bool
    get_random_outputs(const vector<uint64_t>& amounts,
                       const uint64_t& outs_count,
                       vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount>& found_outputs);

    static bool
    commit_tx(const string& tx_blob);

    static bool
    read_mempool();

    static vector<transaction>
    get_mempool_txs();

    static bool
    search_if_payment_made(
            const string& payment_id_str,
            const uint64_t& desired_amount,
            string& tx_hash_with_payment);


    static string
    get_payment_id_as_string(const transaction& tx);

    // definitions of these function are at the end of this file
    // due to forward declaraions of TxSearch
    static bool
    start_tx_search_thread(XmrAccount acc);

    static bool
    ping_search_thread(const string& address);

    static bool
    set_new_searched_blk_no(const string& address, uint64_t new_value);

    static void
    clean_search_thread_map();

};

class TxSearchException: public std::runtime_error
{
    using std::runtime_error::runtime_error;
};


class TxSearch
{
    // how frequently update scanned_block_height in Accounts table
    static constexpr uint64_t UPDATE_SCANNED_HEIGHT_INTERVAL = 10; // seconds

    // how long should the search thread be live after no request
    // are coming from the frontend. For example, when a user finishes
    // using the service.
    static constexpr uint64_t THREAD_LIFE_DURATION           = 10 * 60; // in seconds


    bool continue_search {true};


    uint64_t last_ping_timestamp;

    atomic<uint64_t> searched_blk_no;

    // represents a row in mysql's Accounts table
    shared_ptr<XmrAccount> acc;

    // this manages all mysql queries
    // its better to when each thread has its own mysql connection object.
    // this way if one thread crashes, it want take down
    // connection for the entire service
    shared_ptr<MySqlAccounts> xmr_accounts;

    // address and viewkey for this search thread.
    cryptonote::account_public_address address;
    secret_key viewkey;

public:

    TxSearch(XmrAccount& _acc);

    void
    search();

    void
    stop();

    ~TxSearch();

    void
    set_searched_blk_no(uint64_t new_value);

    void
    ping();

    bool
    still_searching();

};



}
#endif //RESTBED_XMR_CURRENTBLOCKCHAINSTATUS_H
