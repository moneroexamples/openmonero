//
// Created by mwo on 14/12/16.
//

#ifndef RESTBED_XMR_CURRENTBLOCKCHAINSTATUS_H
#define RESTBED_XMR_CURRENTBLOCKCHAINSTATUS_H

#include "mylmdb.h"
#include "tools.h"

#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>


namespace xmreg {


/*
 * This is a thread class
 */
struct CurrentBlockchainStatus {
    static string blockchain_path;

    static atomic<uint64_t> current_height;

    static bool testnet;

    static std::thread m_thread;

    static bool is_running;

    static uint64_t refresh_block_status_every_seconds;

    // since this class monitors current status
    // of the blockchain, its seems logical to
    // make object for accessing the blockchain here
    static xmreg::MicroCore mcore;
    static cryptonote::Blockchain *core_storage;

    static
    void start_monitor_blockchain_thread() {
        if (!is_running) {
            m_thread = std::thread{[]() {
                while (true) {
                    current_height = get_current_blockchain_height();
                    cout << "Check block height: " << current_height << endl;
                    std::this_thread::sleep_for(std::chrono::seconds(refresh_block_status_every_seconds));
                }

            }};

            is_running = true;
        }
    }

    static inline
    uint64_t
    get_current_blockchain_height() {
        return xmreg::MyLMDB::get_blockchain_height(blockchain_path) - 1;
    }

    static void
    set_blockchain_path(const string &path) {
        blockchain_path = path;
    }

    static void
    set_testnet(bool is_testnet) {
        testnet = is_testnet;
    }

    static bool
    init_monero_blockchain() {
        // enable basic monero log output
        xmreg::enable_monero_log();

        // initialize mcore and core_storage
        if (!xmreg::init_blockchain(blockchain_path,
                                    mcore, core_storage)) {
            cerr << "Error accessing blockchain." << endl;
            return false;
        }

        return true;
    }

    static bool
    get_block(uint64_t height, block &blk) {
        return mcore.get_block_by_height(height, blk);
    }

    static bool
    get_block_txs(const block &blk, list <transaction> &blk_txs) {
        // get all transactions in the block found
        // initialize the first list with transaction for solving
        // the block i.e. coinbase tx.
        blk_txs.push_back(blk.miner_tx);

        list <crypto::hash> missed_txs;

        if (!core_storage->get_transactions(blk.tx_hashes, blk_txs, missed_txs)) {
            cerr << "Cant get transactions in block: " << get_block_hash(blk) << endl;
            return false;
        }

        return true;
    }


};

// initialize static variables
atomic<uint64_t>        CurrentBlockchainStatus::current_height{0};
string                  CurrentBlockchainStatus::blockchain_path{"/home/mwo/.blockchain/lmdb"};
bool                    CurrentBlockchainStatus::testnet{false};
bool                    CurrentBlockchainStatus::is_running{false};
std::thread             CurrentBlockchainStatus::m_thread;
uint64_t                CurrentBlockchainStatus::refresh_block_status_every_seconds{60};
xmreg::MicroCore        CurrentBlockchainStatus::mcore;
cryptonote::Blockchain *CurrentBlockchainStatus::core_storage;

}
#endif //RESTBED_XMR_CURRENTBLOCKCHAINSTATUS_H
