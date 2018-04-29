//
// Created by mwo on 8/01/17.
//

#ifndef RESTBED_XMR_TXSEARCH_H
#define RESTBED_XMR_TXSEARCH_H

#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include "MySqlAccounts.h"
#include "OutputInputIdentification.h"

namespace xmreg
{

using namespace std;

class XmrAccount;
class MySqlAccounts;

class TxSearchException: public std::runtime_error
{
    using std::runtime_error::runtime_error;
};



class TxSearch
{
    // how frequently update scanned_block_height in Accounts table
    static constexpr uint64_t UPDATE_SCANNED_HEIGHT_INTERVAL = 5; // seconds

    // how long should the search thread be live after no request
    // are coming from the frontend. For example, when a user finishes
    // using the service.
    static uint64_t thread_search_life; // in seconds

    bool continue_search {true};

    mutex getting_known_outputs_keys;

    uint64_t last_ping_timestamp;

    atomic<uint64_t> searched_blk_no;

    // represents a row in mysql's Accounts table
    shared_ptr<XmrAccount> acc;

    // stores known output public keys.
    // used as a cash to fast look up of
    // our public keys in key images. Saves a lot of
    // mysql queries to Outputs table.
    //
    //          out_pk, amount
    vector<pair<string, uint64_t>> known_outputs_keys;

    // this manages all mysql queries
    // its better to when each thread has its own mysql connection object.
    // this way if one thread crashes, it want take down
    // connection for the entire service
    shared_ptr<MySqlAccounts> xmr_accounts;

    // address and viewkey for this search thread.
    address_parse_info address;
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

    uint64_t
    get_searched_blk_no() const;

    void
    ping();

    bool
    still_searching();

    void
    populate_known_outputs();

    vector<pair<string, uint64_t>>
    get_known_outputs_keys();


    /**
     * Search for our txs in the mempool
     *
     * The method searches for our txs (outputs and inputs)
     * in the mempool. It does basically same what search method
     * The difference is that search method searches in a thread
     * in a blockchain. It does not scan for tx in mempool. This is because
     * it writes what it finds into database for permament storage.
     * However txs in mempool are not permament. Also since we want to
     * give the end user quick update on incoming/outging tx, this method
     * will be executed whenever frontend wants. By default it is every
     * 10 seconds.
     * Also since we dont write here anything to the database, we
     * return a json that will be appended to json produced by get_address_tx
     * and similar function. The outputs here cant be spent anyway. This is
     * only for end user information. The txs found here will be written
     * to database later on by TxSearch thread when they will be added
     * to the blockchain.
     *
     * we pass mempool_txs by copy because we want copy of mempool txs.
     * to avoid worrying about synchronizing threads
     *
     * @return json
     */
    json
    find_txs_in_mempool(vector<pair<uint64_t, transaction>> mempool_txs);

    pair<address_parse_info, secret_key>
    get_xmr_address_viewkey() const;

    static void
    set_search_thread_life(uint64_t life_seconds);

};



}
#endif //RESTBED_XMR_TXSEARCH_H
