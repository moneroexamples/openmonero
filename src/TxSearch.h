//
// Created by mwo on 12/12/16.
//

#ifndef RESTBED_XMR_TXSEARCH_H
#define RESTBED_XMR_TXSEARCH_H



#include "MySqlConnector.h"
#include "tools.h"
#include "mylmdb.h"

#include <mysql++/mysql++.h>
#include <mysql++/ssqls.h>


#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>

namespace xmreg
{

mutex mtx;


class TxSearchException: public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

/*
 * This is a thread class
 */
struct CurrentBlockchainStatus
{
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
    static cryptonote::Blockchain* core_storage;

    static
    void start_monitor_blockchain_thread()
    {
        if (!is_running)
        {
            m_thread = std::thread{ []()
            {
                while (true)
                {
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
    get_current_blockchain_height()
    {
        return xmreg::MyLMDB::get_blockchain_height(blockchain_path) - 1;
    }

    static void
    set_blockchain_path(const string& path)
    {
        blockchain_path = path;
    }

    static void
    set_testnet(bool is_testnet)
    {
        testnet = is_testnet;
    }

    static bool
    init_monero_blockchain()
    {
        // enable basic monero log output
        xmreg::enable_monero_log();

         // initialize mcore and core_storage
        if (!xmreg::init_blockchain(blockchain_path,
                                    mcore, core_storage))
        {
            cerr << "Error accessing blockchain." << endl;
            return false;
        }

        return true;
    }

};

// initialize static variables
atomic<uint64_t>        CurrentBlockchainStatus::current_height {0};
string                  CurrentBlockchainStatus::blockchain_path {"/home/mwo/.blockchain/lmdb"};
bool                    CurrentBlockchainStatus::testnet  {false};
bool                    CurrentBlockchainStatus::is_running  {false};
std::thread             CurrentBlockchainStatus::m_thread;
uint64_t                CurrentBlockchainStatus::refresh_block_status_every_seconds {60};
xmreg::MicroCore        CurrentBlockchainStatus::mcore;
cryptonote::Blockchain* CurrentBlockchainStatus::core_storage;

class TxSearch
{

    bool continue_search {true};

    // represents a row in mysql's Accounts table
    XmrAccount acc;

    // this manages all mysql queries
    MySqlAccounts xmr_accounts;

    // address and viewkey for this search thread.
    account_public_address address;
    secret_key viewkey;



public:

    TxSearch() {}

    TxSearch(XmrAccount& _acc):
            acc {_acc},
            xmr_accounts()
    {

        bool testnet = CurrentBlockchainStatus::testnet;

        if (!xmreg::parse_str_address(acc.address, address, testnet))
        {
            cerr << "Cant parse string address: " << acc.address << endl;
            throw TxSearchException("Cant parse string address: " + acc.address);
        }

        if (!xmreg::parse_str_secret_key(acc.viewkey, viewkey))
        {
            cerr << "Cant parse the private key: " << acc.viewkey << endl;
            throw TxSearchException("Cant parse private key: " + acc.viewkey);
        }

    }

    void
    search()
    {

        // start searching from last block that we searched for
        // this accont
        uint64_t searched_blk_no = acc.scanned_block_height;

        if (searched_blk_no > CurrentBlockchainStatus::current_height)
        {
            throw TxSearchException("searched_blk_no > CurrentBlockchainStatus::current_height");
        }

        while(continue_search)
        {

            //std::lock_guard<std::mutex> lck (mtx);
            cout << " - searching tx of: " << acc << endl;

            // get block cointaining this tx
            block blk;
//
//            if (!mcore->get_block_by_height(tx_blk_height, blk))
//            {
//                cerr << "Cant get block: " << tx_blk_height << endl;
//            }


            std::this_thread::sleep_for(std::chrono::seconds(1));

            if (++searched_blk_no == CurrentBlockchainStatus::current_height)
            {
                std::this_thread::sleep_for(
                        std::chrono::seconds(
                                CurrentBlockchainStatus::refresh_block_status_every_seconds)
                );
            }
        }
    }

    void
    stop()
    {
        cout << "something to stop the thread by setting continue_search=false" << endl;
    }

    ~TxSearch()
    {
        cout << "TxSearch destroyed" << endl;
    }


};

}

#endif //RESTBED_XMR_TXSEARCH_H
