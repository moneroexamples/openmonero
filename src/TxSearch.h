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

namespace xmreg
{

class XmrAccount;
class MySqlAccounts;



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

    // stores known output public keys.
    // used as a cash to fast look up of
    // our public keys in key images. Saves a lot of
    // mysql queries to Outputs table.
    vector<string> known_outputs_keys;

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

    void
    populate_known_outputs();

};



}
#endif //RESTBED_XMR_TXSEARCH_H
