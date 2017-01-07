//
// Created by mwo on 16/12/16.
//

#ifndef RESTBED_XMR_MYSQLACCOUNTS_H
#define RESTBED_XMR_MYSQLACCOUNTS_H

#include "tools.h"
#include "MySqlConnector.h"
#include "CurrentBlockchainStatus.h"


#include <iostream>
#include <memory>
#include <mutex>
#include <thread>


namespace xmreg
{

using namespace mysqlpp;
using namespace std;
using namespace nlohmann;


class XmrTransactionWithOutsAndIns;
class XmrInput;
class XmrOutput;
class XmrTransaction;
class XmrPayment;
class XmrAccount;


static mutex searching_threads_map_mtx;


class MysqlTransactionWithOutsAndIns
{

    shared_ptr<MySqlConnector> conn;

public:

    MysqlTransactionWithOutsAndIns(shared_ptr<MySqlConnector> _conn);

    bool
    select(const uint64_t &address_id, vector<XmrTransactionWithOutsAndIns>& txs);

    bool
    select_for_tx(const uint64_t &tx_id, vector<XmrTransactionWithOutsAndIns>& txs);
};



class MysqlInputs
{

    shared_ptr<MySqlConnector> conn;

public:

    MysqlInputs(shared_ptr<MySqlConnector> _conn);

    bool
    select(const uint64_t& address_id, vector<XmrInput>& ins);

    bool
    select_for_tx(const uint64_t& address_id, vector<XmrInput>& ins);


    bool
    select_for_out(const uint64_t& output_id, vector<XmrInput>& ins);


    uint64_t
    insert(const XmrInput& in_data);

};



class MysqlOutpus
{

    shared_ptr<MySqlConnector> conn;

public:

    MysqlOutpus(shared_ptr<MySqlConnector> _conn);

    bool
    select(const uint64_t& address_id, vector<XmrOutput>& outs);

    bool
    select_for_tx(const uint64_t& tx_id, vector<XmrOutput>& outs);


    bool
    exist(const string& output_public_key_str, XmrOutput& out);



    uint64_t
    insert(const XmrOutput& out_data);

};



class MysqlTransactions
{

    shared_ptr<MySqlConnector> conn;

public:

    MysqlTransactions(shared_ptr<MySqlConnector> _conn);

    bool
    select(const uint64_t& address_id, vector<XmrTransaction>& txs);


    uint64_t
    insert(const XmrTransaction& tx_data);


    bool
    exist(const uint64_t& account_id, const string& tx_hash_str, XmrTransaction& tx);


    uint64_t
    get_total_recieved(const uint64_t& account_id);


};

class MysqlPayments
{

    shared_ptr<MySqlConnector> conn;

public:

    MysqlPayments(shared_ptr<MySqlConnector> _conn);

    bool
    select(const string& address, vector<XmrPayment>& payments);

    bool
    select_by_payment_id(const string& payment_id, vector<XmrPayment>& payments);


    uint64_t
    insert(const XmrPayment& payment_data);


    bool
    update(XmrPayment& payment_orginal, XmrPayment& payment_new);


};

class TxSearch;

class MySqlAccounts
{

    shared_ptr<MySqlConnector> conn;

    shared_ptr<MysqlTransactions> mysql_tx;

    shared_ptr<MysqlOutpus> mysql_out;

    shared_ptr<MysqlInputs> mysql_in;

    shared_ptr<MysqlPayments> mysql_payment;

    shared_ptr<MysqlTransactionWithOutsAndIns> mysql_tx_inout;

    // map that will keep track of search threads. In the
    // map, key is address to which a running thread belongs to.
    // make it static to guarantee only one such map exist.
    static map<string, shared_ptr<TxSearch>> searching_threads;

public:

    MySqlAccounts();


    bool
    select(const string& address, XmrAccount& account);

    bool
    select(const int64_t& acc_id, XmrAccount& account);

    uint64_t
    insert(const string& address, const uint64_t& current_blkchain_height = 0);

    uint64_t
    insert_tx(const XmrTransaction& tx_data);

    uint64_t
    insert_output(const XmrOutput& tx_out);

    uint64_t
    insert_input(const XmrInput& tx_in);

    bool
    select_txs(const string& xmr_address, vector<XmrTransaction>& txs);

    bool
    select_txs(const uint64_t& account_id, vector<XmrTransaction>& txs);


    bool
    select_txs_with_inputs_and_outputs(const uint64_t& account_id,
                                       vector<XmrTransactionWithOutsAndIns>& txs);


    bool
    select_outputs(const uint64_t& account_id, vector<XmrOutput>& outs);

    bool
    select_outputs_for_tx(const uint64_t& tx_id, vector<XmrOutput>& outs);

    bool
    select_inputs(const uint64_t& account_id, vector<XmrInput>& ins);

    bool
    select_inputs_for_tx(const uint64_t& tx_id, vector<XmrTransactionWithOutsAndIns>& ins);

    bool
    select_inputs_for_out(const uint64_t& output_id, vector<XmrInput>& ins);
    bool
    output_exists(const string& output_public_key_str, XmrOutput& out);

    bool
    tx_exists(const uint64_t& account_id, const string& tx_hash_str, XmrTransaction& tx);

    uint64_t
    insert_payment(const XmrPayment& payment);

    bool
    select_payment_by_id(const string& payment_id, vector<XmrPayment>& payments);

    bool
    select_payment_by_address(const string& address, vector<XmrPayment>& payments);

    bool
    select_payment_by_address(const string& address, XmrPayment& payment);

    bool
    update_payment(XmrPayment& payment_orginal, XmrPayment& payment_new);

    uint64_t
    get_total_recieved(const uint64_t& account_id);


    bool
    update(XmrAccount& acc_orginal, XmrAccount& acc_new);


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


#endif //RESTBED_XMR_MYSQLACCOUNTS_H
