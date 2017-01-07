//
// Created by mwo on 14/12/16.
//

#ifndef RESTBED_XMR_CURRENTBLOCKCHAINSTATUS_H
#define RESTBED_XMR_CURRENTBLOCKCHAINSTATUS_H


#include "MicroCore.h"

#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>


namespace xmreg {


class TxSearch;
class XmrAccount;
class MySqlAccounts;


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



};


}
#endif //RESTBED_XMR_CURRENTBLOCKCHAINSTATUS_H
