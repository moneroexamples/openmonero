//
// Created by mwo on 7/01/17.
//

#include "MySqlAccounts.h"

#include "ssqlses.h"


#include <thread>


namespace xmreg
{

MysqlTransactionWithOutsAndIns::MysqlTransactionWithOutsAndIns(shared_ptr<MySqlConnector> _conn)
        : conn{_conn}
{}

bool
MysqlTransactionWithOutsAndIns::select(
        const uint64_t &address_id,
        vector<XmrTransactionWithOutsAndIns>& txs)
{

    Query query = conn->query(XmrTransactionWithOutsAndIns::SELECT_STMT);
    query.parse();

    try
    {
        query.storein(txs, address_id);

        if (!txs.empty())
        {
            return true;
        }
    }
    catch (mysqlpp::Exception &e) {
        MYSQL_EXCEPTION_MSG(e);
    }
    catch (std::exception &e) {
        MYSQL_EXCEPTION_MSG(e);
    }

    return false;
}

bool
MysqlTransactionWithOutsAndIns::select_for_tx(
        const uint64_t &tx_id,
        vector<XmrTransactionWithOutsAndIns>& txs)
{

    Query query = conn->query(XmrTransactionWithOutsAndIns::SELECT_STMT2);
    query.parse();

    try
    {
        query.storein(txs, tx_id);

        if (!txs.empty())
        {
            return true;
        }
    }
    catch (mysqlpp::Exception &e) {
        MYSQL_EXCEPTION_MSG(e);
    }
    catch (std::exception &e) {
        MYSQL_EXCEPTION_MSG(e);
    }

    return false;
}


MysqlInputs::MysqlInputs(shared_ptr<MySqlConnector> _conn): conn {_conn}
{}

bool
MysqlInputs::select(const uint64_t& address_id, vector<XmrInput>& ins)
{

    Query query = conn->query(XmrInput::SELECT_STMT);
    query.parse();

    try
    {
        query.storein(ins, address_id);

        if (!ins.empty())
        {
            return true;
        }
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }

    return false;
}

bool
MysqlInputs::select_for_tx(const uint64_t& address_id, vector<XmrInput>& ins)
{

    Query query = conn->query(XmrInput::SELECT_STMT2);
    query.parse();

    try
    {
        query.storein(ins, address_id);

        if (!ins.empty())
        {
            return true;
        }
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }

    return false;
}


bool
MysqlInputs::select_for_out(const uint64_t& output_id, vector<XmrInput>& ins)
{

    Query query = conn->query(XmrInput::SELECT_STMT3);
    query.parse();

    try
    {
        query.storein(ins, output_id);

        if (!ins.empty())
        {
            return true;
        }
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }

    return false;
}


uint64_t
MysqlInputs::insert(const XmrInput& in_data)
{

//        static shared_ptr<Query> query;
//
//        if (!query)
//        {
//            Query q = MySqlConnector::getInstance().query(XmrInput::INSERT_STMT);
//            q.parse();
//            query = shared_ptr<Query>(new Query(q));
//        }


    Query query = conn->query(XmrInput::INSERT_STMT);
    query.parse();

    // cout << query << endl;

    try
    {
        SimpleResult sr = query.execute(in_data.account_id,
                                        in_data.tx_id,
                                        in_data.output_id,
                                        in_data.key_image,
                                        in_data.amount,
                                        in_data.timestamp);

        if (sr.rows() == 1)
            return sr.insert_id();

    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        return 0;
    }

    return 0;
}



MysqlOutpus::MysqlOutpus(shared_ptr<MySqlConnector> _conn): conn {_conn}
{}

bool
MysqlOutpus::select(const uint64_t& address_id, vector<XmrOutput>& outs)
{
    Query query = conn->query(XmrOutput::SELECT_STMT);
    query.parse();

    try
    {
        query.storein(outs, address_id);

        if (!outs.empty())
        {
            return true;
        }
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }

    return false;
}

bool
MysqlOutpus::select_for_tx(const uint64_t& tx_id, vector<XmrOutput>& outs)
{
    Query query = conn->query(XmrOutput::SELECT_STMT2);
    query.parse();

    try
    {
        query.storein(outs, tx_id);

        if (!outs.empty())
        {
            return true;
        }
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }

    return false;
}


bool
MysqlOutpus::exist(const string& output_public_key_str, XmrOutput& out)
{

    Query query = conn->query(XmrOutput::EXIST_STMT);
    query.parse();

    try
    {

        vector<XmrOutput> outs;

        query.storein(outs, output_public_key_str);

        if (outs.empty())
        {
            return false;
        }

        out = outs.at(0);

    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }

    return true;
}



uint64_t
MysqlOutpus::insert(const XmrOutput& out_data)
{

//        static shared_ptr<Query> query;
//
//        if (!query)
//        {
//            Query q = MySqlConnector::getInstance().query(XmrOutput::INSERT_STMT);
//            q.parse();
//            query = shared_ptr<Query>(new Query(q));
//        }


    Query query = conn->query(XmrOutput::INSERT_STMT);
    query.parse();

    // cout << query << endl;

    try
    {
        SimpleResult sr = query.execute(out_data.account_id,
                                        out_data.tx_id,
                                        out_data.out_pub_key,
                                        out_data.tx_pub_key,
                                        out_data.amount,
                                        out_data.global_index,
                                        out_data.out_index,
                                        out_data.mixin,
                                        out_data.timestamp);

        if (sr.rows() == 1)
            return sr.insert_id();

    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        return 0;
    }

    return 0;
}



MysqlTransactions::MysqlTransactions(shared_ptr<MySqlConnector> _conn): conn {_conn}
{}

bool
MysqlTransactions::select(const uint64_t& address_id, vector<XmrTransaction>& txs)
{
//
//        static shared_ptr<Query> query;
//
//        if (!query)
//        {
//            Query q = MySqlConnector::getInstance().query(
//                    XmrTransaction::SELECT_STMT);
//            q.parse();
//            query = shared_ptr<Query>(new Query(q));
//        }

    Query query = conn->query(XmrTransaction::SELECT_STMT);
    query.parse();

    try
    {
        query.storein(txs, address_id);

        if (!txs.empty())
        {
            return true;
        }
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }

    return false;
}


uint64_t
MysqlTransactions::insert(const XmrTransaction& tx_data)
{

//        static shared_ptr<Query> query;
//
//        if (!query)
//        {
//            Query q = MySqlConnector::getInstance().query(XmrTransaction::INSERT_STMT);
//            q.parse();
//            query = shared_ptr<Query>(new Query(q));
//        }


    Query query = conn->query(XmrTransaction::INSERT_STMT);
    query.parse();

    // cout << query << endl;

    try
    {
        SimpleResult sr = query.execute(tx_data.hash,
                                        tx_data.prefix_hash,
                                        tx_data.account_id,
                                        tx_data.total_received,
                                        tx_data.total_sent,
                                        tx_data.unlock_time,
                                        tx_data.height,
                                        tx_data.coinbase,
                                        tx_data.payment_id,
                                        tx_data.mixin,
                                        tx_data.timestamp);

        if (sr.rows() == 1)
            return sr.insert_id();

    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        return 0;
    }

    return 0;
}


bool
MysqlTransactions::exist(const uint64_t& account_id, const string& tx_hash_str, XmrTransaction& tx)
{

    Query query = conn->query(XmrTransaction::EXIST_STMT);
    query.parse();

    try
    {

        vector<XmrTransaction> outs;

        query.storein(outs, account_id, tx_hash_str);

        if (outs.empty())
        {
            return false;
        }

        tx = outs.at(0);

    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }

    return true;
}


uint64_t
MysqlTransactions::get_total_recieved(const uint64_t& account_id)
{
    Query query = conn->query(XmrTransaction::SUM_XMR_RECIEVED);
    query.parse();

    try
    {
        StoreQueryResult sqr = query.store(account_id);

        if (!sqr)
        {
            return 0;
        }

        Row row = sqr.at(0);

        return row["total_received"];
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        return 0;
    }
}

MysqlPayments::MysqlPayments(shared_ptr<MySqlConnector> _conn): conn {_conn}
{}

bool
MysqlPayments::select(const string& address, vector<XmrPayment>& payments)
{

    Query query = conn->query(XmrPayment::SELECT_STMT);
    query.parse();

    try
    {
        query.storein(payments, address);

        if (!payments.empty())
        {
            return true;
        }
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }

    return false;
}

bool
MysqlPayments::select_by_payment_id(const string& payment_id, vector<XmrPayment>& payments)
{

    Query query = conn->query(XmrPayment::SELECT_STMT2);
    query.parse();

    try
    {
        query.storein(payments, payment_id);

        if (!payments.empty())
        {
            return true;
        }
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }

    return false;
}


uint64_t
MysqlPayments::insert(const XmrPayment& payment_data)
{

    Query query = conn->query(XmrPayment::INSERT_STMT);
    query.parse();

    // cout << query << endl;

    try
    {
        SimpleResult sr = query.execute(payment_data.address,
                                        payment_data.payment_id,
                                        payment_data.tx_hash,
                                        payment_data.request_fulfilled,
                                        payment_data.payment_address,
                                        payment_data.import_fee);

        if (sr.rows() == 1)
            return sr.insert_id();

    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        return 0;
    }

    return 0;
}


bool
MysqlPayments::update(XmrPayment& payment_orginal, XmrPayment& payment_new)
{

    Query query = conn->query();

    try
    {
        query.update(payment_orginal, payment_new);

        SimpleResult sr = query.execute();

        if (sr.rows() == 1)
            return true;
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        return false;
    }

    return false;
}

MySqlAccounts::MySqlAccounts()
{
    // create connection to the mysql
    conn            = make_shared<MySqlConnector>();

    // use same connection when working with other tables
    mysql_tx        = make_shared<MysqlTransactions>(conn);
    mysql_out       = make_shared<MysqlOutpus>(conn);
    mysql_in        = make_shared<MysqlInputs>(conn);
    mysql_payment   = make_shared<MysqlPayments>(conn);
    mysql_tx_inout  = make_shared<MysqlTransactionWithOutsAndIns>(conn);

}


bool
MySqlAccounts::select(const string& address, XmrAccount& account)
{

//        static shared_ptr<Query> query;
//
//        if (!query)
//        {
//            Query q = MySqlConnector::getInstance().query(XmrAccount::SELECT_STMT);
//            q.parse();
//            query = shared_ptr<Query>(new Query(q));
//        }

    Query query = conn->query(XmrAccount::SELECT_STMT);
    query.parse();

    try
    {
        vector<XmrAccount> res;
        query.storein(res, address);

        if (!res.empty())
        {
            account = res.at(0);
            return true;
        }

    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }

    return false;
}

bool
MySqlAccounts::select(const int64_t& acc_id, XmrAccount& account)
{

    //static shared_ptr<Query> query;

//        if (!query)
//        {
//            Query q = MySqlConnector::getInstance().query(XmrAccount::SELECT_STMT2);
//            q.parse();
//            query = shared_ptr<Query>(new Query(q));
//        }

    Query query = conn->query(XmrAccount::SELECT_STMT2);
    query.parse();

    try
    {
        vector<XmrAccount> res;
        query.storein(res, acc_id);

        if (!res.empty())
        {
            account = res.at(0);
            return true;
        }

    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }

    return false;
}

uint64_t
MySqlAccounts::insert(const string& address, const uint64_t& current_blkchain_height)
{

    //    static shared_ptr<Query> query;

//        if (!query)
//        {
//            Query q = MySqlConnector::getInstance().query(XmrAccount::INSERT_STMT);
//            q.parse();
//            query = shared_ptr<Query>(new Query(q));
//        }


    Query query = conn->query(XmrAccount::INSERT_STMT);
    query.parse();

    // cout << query << endl;

    try
    {
        // scanned_block_height and start_height are
        // set to current blockchain height
        // when account is created.

        SimpleResult sr = query.execute(address,
                                        current_blkchain_height,
                                        current_blkchain_height);

        if (sr.rows() == 1)
            return sr.insert_id();

    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        return 0;
    }

    return 0;
}

uint64_t
MySqlAccounts::insert_tx(const XmrTransaction& tx_data)
{
    return mysql_tx->insert(tx_data);
}

uint64_t
MySqlAccounts::insert_output(const XmrOutput& tx_out)
{
    return mysql_out->insert(tx_out);
}

uint64_t
MySqlAccounts::insert_input(const XmrInput& tx_in)
{
    return mysql_in->insert(tx_in);
}

bool
MySqlAccounts::select_txs(const string& xmr_address, vector<XmrTransaction>& txs)
{
    // having address first get its address_id


    // a placeholder for exciting or new account data
    xmreg::XmrAccount acc;

    // select this account if its existing one
    if (!select(xmr_address, acc))
    {
        cerr << "Address" << xmr_address << "does not exist in database" << endl;
        return false;
    }

    return mysql_tx->select(acc.id, txs);
}

bool
MySqlAccounts::select_txs(const uint64_t& account_id, vector<XmrTransaction>& txs)
{
    return mysql_tx->select(account_id, txs);
}


bool
MySqlAccounts::select_txs_with_inputs_and_outputs(const uint64_t& account_id,
                                   vector<XmrTransactionWithOutsAndIns>& txs)
{
    return mysql_tx_inout->select(account_id, txs);
}


bool
MySqlAccounts::select_outputs(const uint64_t& account_id, vector<XmrOutput>& outs)
{
    return mysql_out->select(account_id, outs);
}

bool
MySqlAccounts::select_outputs_for_tx(const uint64_t& tx_id, vector<XmrOutput>& outs)
{
    return mysql_out->select_for_tx(tx_id, outs);
}

bool
MySqlAccounts::select_inputs(const uint64_t& account_id, vector<XmrInput>& ins)
{
    return mysql_in->select(account_id, ins);
}

bool
MySqlAccounts::select_inputs_for_tx(const uint64_t& tx_id, vector<XmrTransactionWithOutsAndIns>& ins)
{
    return mysql_tx_inout->select_for_tx(tx_id, ins);
}

bool
MySqlAccounts::select_inputs_for_out(const uint64_t& output_id, vector<XmrInput>& ins)
{
    return mysql_in->select_for_out(output_id, ins);
}

bool
MySqlAccounts::output_exists(const string& output_public_key_str, XmrOutput& out)
{
    return mysql_out->exist(output_public_key_str, out);
}

bool
MySqlAccounts::tx_exists(const uint64_t& account_id, const string& tx_hash_str, XmrTransaction& tx)
{
    return mysql_tx->exist(account_id, tx_hash_str, tx);
}

uint64_t
MySqlAccounts::insert_payment(const XmrPayment& payment)
{
    return mysql_payment->insert(payment);
}

bool
MySqlAccounts::select_payment_by_id(const string& payment_id, vector<XmrPayment>& payments)
{
    return mysql_payment->select_by_payment_id(payment_id, payments);
}

bool
MySqlAccounts::select_payment_by_address(const string& address, vector<XmrPayment>& payments)
{
    return mysql_payment->select(address, payments);
}

bool
MySqlAccounts::select_payment_by_address(const string& address, XmrPayment& payment)
{

    vector<XmrPayment> payments;

    bool r = mysql_payment->select(address, payments);

    if (!r)
    {
        return false;
    }

    if (payments.empty())
    {
        return false;
    }

    // always get last payment details.
    payment = payments.back();

    return r;
}

bool
MySqlAccounts::update_payment(XmrPayment& payment_orginal, XmrPayment& payment_new)
{
    return mysql_payment->update(payment_orginal, payment_new);
}

uint64_t
MySqlAccounts::get_total_recieved(const uint64_t& account_id)
{
    return mysql_tx->get_total_recieved(account_id);
}


bool
MySqlAccounts::update(XmrAccount& acc_orginal, XmrAccount& acc_new)
{

    Query query = conn->query();

    try
    {
        query.update(acc_orginal, acc_new);

        SimpleResult sr = query.execute();

        if (sr.rows() == 1)
            return true;
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        return false;
    }

    return false;
}



bool
MySqlAccounts::start_tx_search_thread(XmrAccount acc)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (searching_threads.count(acc.address) > 0)
    {
        // thread for this address exist, dont make new one
        cout << "Thread exisist, dont make new one" << endl;
        return false;
    }

    // make a tx_search object for the given xmr account
    searching_threads[acc.address] = make_shared<TxSearch>(acc);

    // start the thread for the created object
    std::thread t1 {&TxSearch::search, searching_threads[acc.address].get()};
    t1.detach();

    return true;
}

bool
MySqlAccounts::ping_search_thread(const string& address)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (searching_threads.count(address) == 0)
    {
        // thread does not exist
        cout << "does not exist" << endl;
        return false;
    }

    searching_threads[address].get()->ping();

    return true;
}

bool
MySqlAccounts::set_new_searched_blk_no(const string& address, uint64_t new_value)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (searching_threads.count(address) == 0)
    {
        // thread does not exist
        cout << " thread does not exist" << endl;
        return false;
    }

    searching_threads[address].get()->set_searched_blk_no(new_value);

    return true;
}


void
MySqlAccounts::clean_search_thread_map()
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    for (auto st: searching_threads)
    {
        if (st.second->still_searching() == false)
        {
            cout << st.first << " still searching: " << st.second->still_searching() << endl;
            searching_threads.erase(st.first);
        }
    }
}



TxSearch::TxSearch(XmrAccount& _acc)
{
    acc = make_shared<XmrAccount>(_acc);

    xmr_accounts = make_shared<MySqlAccounts>();

    bool testnet = CurrentBlockchainStatus::testnet;

    if (!xmreg::parse_str_address(acc->address, address, testnet))
    {
        cerr << "Cant parse string address: " << acc->address << endl;
        throw TxSearchException("Cant parse string address: " + acc->address);
    }

    if (!xmreg::parse_str_secret_key(acc->viewkey, viewkey))
    {
        cerr << "Cant parse the private key: " << acc->viewkey << endl;
        throw TxSearchException("Cant parse private key: " + acc->viewkey);
    }

    // start searching from last block that we searched for
    // this accont
    searched_blk_no = acc->scanned_block_height;

    ping();
}

void
TxSearch::search()
{

    if (searched_blk_no > CurrentBlockchainStatus::current_height)
    {
        throw TxSearchException("searched_blk_no > CurrentBlockchainStatus::current_height");
    }

    uint64_t current_timestamp = chrono::duration_cast<chrono::seconds>(
            chrono::system_clock::now().time_since_epoch()).count();


    uint64_t loop_idx {0};

    while(continue_search)
    {
        ++loop_idx;

        uint64_t loop_timestamp {current_timestamp};

        if (loop_idx % 2 == 0)
        {
            // get loop time every second iteration. no need to call it
            // all the time.
            loop_timestamp = chrono::duration_cast<chrono::seconds>(
                    chrono::system_clock::now().time_since_epoch()).count();

            //cout << "loop_timestamp: " <<  loop_timestamp << endl;
            //cout << "last_ping_timestamp: " <<  last_ping_timestamp << endl;
            //cout << "loop_timestamp - last_ping_timestamp: " <<  (loop_timestamp - last_ping_timestamp) << endl;

            if (loop_timestamp - last_ping_timestamp > THREAD_LIFE_DURATION)
            {
                // also check if we caught up with current blockchain height
                if (searched_blk_no == CurrentBlockchainStatus::current_height)
                {
                    stop();
                    continue;
                }
            }
        }



        if (searched_blk_no > CurrentBlockchainStatus::current_height) {
            fmt::print("searched_blk_no {:d} and current_height {:d}\n",
                       searched_blk_no, CurrentBlockchainStatus::current_height);

            std::this_thread::sleep_for(
                    std::chrono::seconds(
                            CurrentBlockchainStatus::refresh_block_status_every_seconds)
            );

            continue;
        }

        //
        //cout << " - searching tx of: " << acc << endl;

        // get block cointaining this tx
        block blk;

        if (!CurrentBlockchainStatus::get_block(searched_blk_no, blk)) {
            cerr << "Cant get block of height: " + to_string(searched_blk_no) << endl;
            //searched_blk_no = -2; // just go back one block, and retry
            continue;
        }

        // for each tx in the given block look, get ouputs


        list <cryptonote::transaction> blk_txs;

        if (!CurrentBlockchainStatus::get_block_txs(blk, blk_txs)) {
            throw TxSearchException("Cant get transactions in block: " + to_string(searched_blk_no));
        }


        if (searched_blk_no % 100 == 0)
        {
            // print status every 10th block

            fmt::print(" - searching block  {:d} of hash {:s} \n",
                       searched_blk_no, pod_to_hex(get_block_hash(blk)));
        }



        // searching for our incoming and outgoing xmr has two componotes.
        //
        // FIRST. to search for the incoming xmr, we use address, viewkey and
        // outputs public key. Its stright forward.
        // second. searching four our spendings is trickier, as we dont have
        //
        // SECOND. But what we can do, we can look for condidate spend keys.
        // and this can be achieved by checking if any mixin in associated with
        // the given image, is our output. If it is our output, than we assume
        // its our key image (i.e. we spend this output). Off course this is only
        // assumption as our outputs can be used in key images of others for their
        // mixin purposes. Thus, we sent to the front end, the list of key images
        // that we think are yours, and the frontend, because it has spend key,
        // can filter out false positives.
        for (transaction& tx: blk_txs)
        {

            crypto::hash tx_hash         = get_transaction_hash(tx);
            crypto::hash tx_prefix_hash  = get_transaction_prefix_hash(tx);

            string tx_hash_str           = pod_to_hex(tx_hash);
            string tx_prefix_hash_str    = pod_to_hex(tx_prefix_hash);

            bool is_coinbase_tx = is_coinbase(tx);

            vector<uint64_t> amount_specific_indices;

            // cout << pod_to_hex(tx_hash) << endl;

            public_key tx_pub_key = xmreg::get_tx_pub_key_from_received_outs(tx);

            //          <public_key  , amount  , out idx>
            vector<tuple<txout_to_key, uint64_t, uint64_t>> outputs;

            outputs = get_ouputs_tuple(tx);

            // for each output, in a tx, check if it belongs
            // to the given account of specific address and viewkey

            // public transaction key is combined with our viewkey
            // to create, so called, derived key.
            key_derivation derivation;

            if (!generate_key_derivation(tx_pub_key, viewkey, derivation))
            {
                cerr << "Cant get derived key for: "  << "\n"
                     << "pub_tx_key: " << tx_pub_key << " and "
                     << "prv_view_key" << viewkey << endl;

                throw TxSearchException("");
            }

            uint64_t total_received {0};


            // FIRST component: Checking for our outputs.

            //     <out_pub_key, amount  , index in tx>
            vector<tuple<string, uint64_t, uint64_t>> found_mine_outputs;

            for (auto& out: outputs)
            {
                txout_to_key txout_k      = std::get<0>(out);
                uint64_t amount           = std::get<1>(out);
                uint64_t output_idx_in_tx = std::get<2>(out);

                // get the tx output public key
                // that normally would be generated for us,
                // if someone had sent us some xmr.
                public_key generated_tx_pubkey;

                derive_public_key(derivation,
                                  output_idx_in_tx,
                                  address.m_spend_public_key,
                                  generated_tx_pubkey);

                // check if generated public key matches the current output's key
                bool mine_output = (txout_k.key == generated_tx_pubkey);


                //cout  << "Chekcing output: "  << pod_to_hex(txout_k.key) << " "
                //      << "mine_output: " << mine_output << endl;


                // if mine output has RingCT, i.e., tx version is 2
                // need to decode its amount. otherwise its zero.
                if (mine_output && tx.version == 2)
                {
                    // initialize with regular amount
                    uint64_t rct_amount = amount;



                    // cointbase txs have amounts in plain sight.
                    // so use amount from ringct, only for non-coinbase txs
                    if (!is_coinbase_tx)
                    {
                        bool r;

                        r = decode_ringct(tx.rct_signatures,
                                          tx_pub_key,
                                          viewkey,
                                          output_idx_in_tx,
                                          tx.rct_signatures.ecdhInfo[output_idx_in_tx].mask,
                                          rct_amount);

                        if (!r)
                        {
                            cerr << "Cant decode ringCT!" << endl;
                            throw TxSearchException("Cant decode ringCT!");
                        }

                        rct_amount = amount;
                    }

                    amount = rct_amount;

                } // if (mine_output && tx.version == 2)

                if (mine_output)
                {

                    string out_key_str = pod_to_hex(txout_k.key);

                    // found an output associated with the given address and viewkey
                    string msg = fmt::format("block: {:d}, tx_hash:  {:s}, output_pub_key: {:s}\n",
                                             searched_blk_no,
                                             pod_to_hex(tx_hash),
                                             out_key_str);

                    cout << msg << endl;


                    total_received += amount;

                    found_mine_outputs.emplace_back(out_key_str,
                                                    amount,
                                                    output_idx_in_tx);
                }

            } // for (const auto& out: outputs)

            DateTime blk_timestamp_mysql_format
                    = XmrTransaction::timestamp_to_DateTime(blk.timestamp);

            uint64_t tx_mysql_id {0};

            if (!found_mine_outputs.empty())
            {

                // before adding this tx and its outputs to mysql
                // check if it already exists. So that we dont
                // do it twice.

                XmrTransaction tx_data;

                if (xmr_accounts->tx_exists(acc->id, tx_hash_str, tx_data))
                {
                    cout << "\nTransaction " << tx_hash_str
                         << " already present in mysql"
                         << endl;

                    continue;
                }


                tx_data.hash           = tx_hash_str;
                tx_data.prefix_hash    = tx_prefix_hash_str;
                tx_data.account_id     = acc->id;
                tx_data.total_received = total_received;
                tx_data.total_sent     = 0; // at this stage we don't have any
                // info about spendings
                tx_data.unlock_time    = 0;
                tx_data.height         = searched_blk_no;
                tx_data.coinbase       = is_coinbase_tx;
                tx_data.payment_id     = CurrentBlockchainStatus::get_payment_id_as_string(tx);
                tx_data.mixin          = get_mixin_no(tx) - 1;
                tx_data.timestamp      = blk_timestamp_mysql_format;

                // insert tx_data into mysql's Transactions table
                tx_mysql_id = xmr_accounts->insert_tx(tx_data);

                // get amount specific (i.e., global) indices of outputs

                if (!CurrentBlockchainStatus::get_amount_specific_indices(tx_hash,
                                                                          amount_specific_indices))
                {
                    cerr << "cant get_amount_specific_indices!" << endl;
                    throw TxSearchException("cant get_amount_specific_indices!");
                }

                if (tx_mysql_id == 0)
                {
                    //cerr << "tx_mysql_id is zero!" << endl;
                    //throw TxSearchException("tx_mysql_id is zero!");
                }

                // now add the found outputs into Outputs tables

                for (auto &out_k_idx: found_mine_outputs)
                {
                    XmrOutput out_data;

                    out_data.account_id   = acc->id;
                    out_data.tx_id        = tx_mysql_id;
                    out_data.out_pub_key  = std::get<0>(out_k_idx);
                    out_data.tx_pub_key   = pod_to_hex(tx_pub_key);
                    out_data.amount       = std::get<1>(out_k_idx);
                    out_data.out_index    = std::get<2>(out_k_idx);
                    out_data.global_index = amount_specific_indices.at(out_data.out_index);
                    out_data.mixin        = tx_data.mixin;
                    out_data.timestamp    = tx_data.timestamp;

                    // insert output into mysql's outputs table
                    uint64_t out_mysql_id = xmr_accounts->insert_output(out_data);

                    if (out_mysql_id == 0)
                    {
                        //cerr << "out_mysql_id is zero!" << endl;
                        //throw TxSearchException("out_mysql_id is zero!");
                    }
                } // for (auto &out_k_idx: found_mine_outputs)



                // once tx and outputs were added, update Accounts table

                XmrAccount updated_acc = *acc;

                updated_acc.total_received = acc->total_received + tx_data.total_received;

                if (xmr_accounts->update(*acc, updated_acc))
                {
                    // if success, set acc to updated_acc;
                    *acc = updated_acc;
                }

            } // if (!found_mine_outputs.empty())



            // SECOND component: Checking for our key images, i.e., inputs.

            vector<txin_to_key> input_key_imgs = xmreg::get_key_images(tx);

            // here we will keep what we find.
            vector<XmrInput> inputs_found;

            // make timescale maps for mixins in input
            for (const txin_to_key& in_key: input_key_imgs)
            {
                // get absolute offsets of mixins
                std::vector<uint64_t> absolute_offsets
                        = cryptonote::relative_output_offsets_to_absolute(
                                in_key.key_offsets);

                // get public keys of outputs used in the mixins that match to the offests
                std::vector<cryptonote::output_data_t> mixin_outputs;


                if (!CurrentBlockchainStatus::get_output_keys(in_key.amount,
                                                              absolute_offsets,
                                                              mixin_outputs))
                {
                    cerr << "Mixins key images not found" << endl;
                    continue;
                }


                // for each found output public key find check if its ours or not
                for (const cryptonote::output_data_t& output_data: mixin_outputs)
                {
                    string output_public_key_str = pod_to_hex(output_data.pubkey);

                    XmrOutput out;

                    if (xmr_accounts->output_exists(output_public_key_str, out))
                    {
                        cout << "input uses some mixins which are our outputs"
                             << out << endl;

                        // seems that this key image is ours.
                        // so save it to database for later use.



                        XmrInput in_data;

                        in_data.account_id  = acc->id;
                        in_data.tx_id       = 0; // for now zero, later we set it
                        in_data.output_id   = out.id;
                        in_data.key_image   = pod_to_hex(in_key.k_image);
                        in_data.amount      = in_key.amount;
                        in_data.timestamp   = blk_timestamp_mysql_format;

                        inputs_found.push_back(in_data);

                        // a key image has only one real mixin. Rest is fake.
                        // so if we find a candidate, break the search.

                        // break;

                    } // if (xmr_accounts->output_exists(output_public_key_str, out))

                } // for (const cryptonote::output_data_t& output_data: outputs)

            } // for (const txin_to_key& in_key: input_key_imgs)


            if (!inputs_found.empty())
            {
                // seems we have some inputs found. time
                // to write it to mysql. But first,
                // check if this tx is written in mysql.


                // calculate how much we preasumply spent.
                uint64_t total_sent {0};

                for (const XmrInput& in_data: inputs_found)
                {
                    total_sent += in_data.amount;
                }


                if (tx_mysql_id == 0)
                {
                    // this txs hasnt been seen in step first.
                    // it means that it only contains potentially our
                    // key images. It does not have our outputs.
                    // so write it to mysql as ours, with
                    // total received of 0.

                    XmrTransaction tx_data;

                    tx_data.hash           = tx_hash_str;
                    tx_data.prefix_hash    = tx_prefix_hash_str;
                    tx_data.account_id     = acc->id;
                    tx_data.total_received = 0;
                    tx_data.total_sent     = total_sent;
                    tx_data.unlock_time    = 0;
                    tx_data.height         = searched_blk_no;
                    tx_data.coinbase       = is_coinbase(tx);
                    tx_data.payment_id     = CurrentBlockchainStatus::get_payment_id_as_string(tx);
                    tx_data.mixin          = get_mixin_no(tx) - 1;
                    tx_data.timestamp      = blk_timestamp_mysql_format;

                    // insert tx_data into mysql's Transactions table
                    tx_mysql_id = xmr_accounts->insert_tx(tx_data);

                    if (tx_mysql_id == 0)
                    {
                        //cerr << "tx_mysql_id is zero!" << endl;
                        //throw TxSearchException("tx_mysql_id is zero!");
                    }
                }

            } //  if (!inputs_found.empty())


            // save all input found into database
            for (XmrInput& in_data: inputs_found)
            {
                in_data.tx_id = tx_mysql_id;
                uint64_t in_mysql_id = xmr_accounts->insert_input(in_data);
            }


        } // for (const transaction& tx: blk_txs)


        if ((loop_timestamp - current_timestamp > UPDATE_SCANNED_HEIGHT_INTERVAL)
            || searched_blk_no == CurrentBlockchainStatus::current_height)
        {
            // update scanned_block_height every given interval
            // or when we reached top of the blockchain

            XmrAccount updated_acc = *acc;

            updated_acc.scanned_block_height = searched_blk_no;

            if (xmr_accounts->update(*acc, updated_acc))
            {
                // iff success, set acc to updated_acc;
                cout << "scanned_block_height updated"  << endl;
                *acc = updated_acc;
            }

            current_timestamp = loop_timestamp;
        }

        ++searched_blk_no;

    } // while(continue_search)
}

void
TxSearch::stop()
{
    cout << "Stoping the thread by setting continue_search=false" << endl;
    continue_search = false;
}

TxSearch::~TxSearch()
{
    cout << "TxSearch destroyed" << endl;
}

void
TxSearch::set_searched_blk_no(uint64_t new_value)
{
    searched_blk_no = new_value;
}


void
TxSearch::ping()
{
    cout << "new last_ping_timestamp: " << last_ping_timestamp << endl;

    last_ping_timestamp = chrono::duration_cast<chrono::seconds>(
            chrono::system_clock::now().time_since_epoch()).count();
}

bool
TxSearch::still_searching()
{
    return continue_search;
}




}