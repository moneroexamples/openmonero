//
// Created by mwo on 7/01/17.
//

#define MYSQLPP_SSQLS_NO_STATICS 1

#include "MySqlAccounts.h"
#include "TxSearch.h"
#include "CurrentBlockchainStatus.h"

#include "ssqlses.h"

namespace xmreg
{


MysqlInputs::MysqlInputs(shared_ptr<MySqlConnector> _conn)
        : conn {_conn}
{}

bool
MysqlInputs::select(const uint64_t& address_id, vector<XmrInput>& ins)
{

    Query query = conn->query(XmrInput::SELECT_STMT);
    query.parse();

    try
    {
        query.storein(ins, address_id);

        return !ins.empty();
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
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

        return !ins.empty();
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
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

        return !ins.empty();
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }

    return false;
}


uint64_t
MysqlInputs::insert(const XmrInput& in_data)
{

    Query query = conn->query(XmrInput::INSERT_STMT);
    query.parse();

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
        //throw  e;
        //return 0;
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
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

        return !outs.empty();
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }

    return false;
}


bool
MysqlOutpus::select(const uint64_t& out_id, XmrOutput& out)
{
    Query query = conn->query(XmrOutput::SELECT_STMT3);
    query.parse();

    try
    {
        vector<XmrOutput> outs;

        query.storein(outs, out_id);

        if (!outs.empty())
        {
            out = outs.at(0);
            return true;
        }
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
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

        return !outs.empty();
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
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
            return false;

        out = outs.at(0);

    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }

    return true;
}



uint64_t
MysqlOutpus::insert(const XmrOutput& out_data)
{
    Query query = conn->query(XmrOutput::INSERT_STMT);
    query.parse();

    // cout << query << endl;

    try
    {
        SimpleResult sr = query.execute(out_data.account_id,
                                        out_data.tx_id,
                                        out_data.out_pub_key,
                                        out_data.tx_pub_key,
                                        out_data.rct_outpk,
                                        out_data.rct_mask,
                                        out_data.rct_amount,
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
        //throw  e;
        //return 0;
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }

    return 0;
}



MysqlTransactions::MysqlTransactions(shared_ptr<MySqlConnector> _conn): conn {_conn}
{}

bool
MysqlTransactions::select(const uint64_t& address_id, vector<XmrTransaction>& txs)
{
    Query query = conn->query(XmrTransaction::SELECT_STMT);
    query.parse();

    try
    {
        query.storein(txs, address_id);

        return !txs.empty();
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }

    return false;
}


uint64_t
MysqlTransactions::insert(const XmrTransaction& tx_data)
{
    Query query = conn->query(XmrTransaction::INSERT_STMT);
    query.parse();

    // cout << query << endl;

    try
    {
        SimpleResult sr = query.execute(tx_data.hash,
                                        tx_data.prefix_hash,
                                        tx_data.tx_pub_key,
                                        tx_data.account_id,
                                        tx_data.blockchain_tx_id,
                                        tx_data.total_received,
                                        tx_data.total_sent,
                                        tx_data.unlock_time,
                                        tx_data.height,
                                        tx_data.coinbase,
                                        tx_data.is_rct,
                                        tx_data.rct_type,
                                        tx_data.spendable,
                                        tx_data.payment_id,
                                        tx_data.mixin,
                                        tx_data.timestamp);

        if (sr.rows() == 1)
            return sr.insert_id();

    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
        //return 0;
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }

    return 0;
}

uint64_t
MysqlTransactions::mark_spendable(const uint64_t& tx_id_no)
{
    Query query = conn->query(XmrTransaction::MARK_AS_SPENDABLE_STMT);
    query.parse();

    try
    {
        SimpleResult sr = query.execute(tx_id_no);

        return sr.rows();
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        // throw  e;
        //return 0;
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }

    return 0;
}


uint64_t
MysqlTransactions::delete_tx(const uint64_t& tx_id_no)
{
    Query query = conn->query(XmrTransaction::DELETE_STMT);
    query.parse();

    try
    {
        SimpleResult sr = query.execute(tx_id_no);

        return sr.rows();
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
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
            return false;

        tx = outs.at(0);

    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
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
            return 0;

        Row row = sqr.at(0);

        return row["total_received"];
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }

    return 0;
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

        return !payments.empty();
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
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

        return !payments.empty();
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
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
        //throw  e;
        //return 0;
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
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

        return sr.rows() == 1;
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
        //return false;
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
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
}


bool
MySqlAccounts::select(const string& address, XmrAccount& account)
{

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
        //throw  e;
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }

    return false;
}

bool
MySqlAccounts::select(const int64_t& acc_id, XmrAccount& account)
{

    if (!conn->connect())
    {
        cerr << __FUNCTION__ << ":" << __LINE__
             <<  " failed connecting to mysql"
             << endl;
        return false;
    }

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
        //throw  e;
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }

    return false;
}

uint64_t
MySqlAccounts::insert(const string& address,
                      const string& viewkey_hash,
                      DateTime const& current_blkchain_timestamp,
                      uint64_t const& current_blkchain_height)
{

    Query query = conn->query(XmrAccount::INSERT_STMT);
    query.parse();

    // cout << query << endl;

    try
    {
        // scanned_block_height and start_height are
        // set to current blockchain height
        // when account is created.

        //cout << "insert address: " << address << endl;

        SimpleResult sr = query.execute(address,
                                        viewkey_hash,
                                        current_blkchain_height,
                                        current_blkchain_timestamp,
                                        current_blkchain_height);

        if (sr.rows() == 1)
            return sr.insert_id();

    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
        //return 0;
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
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
MySqlAccounts::select_txs_for_account_spendability_check(
        const uint64_t& account_id,
        vector<XmrTransaction>& txs)
{
    vector<XmrTransaction> txs_tmp;

    if (!select_txs(account_id, txs_tmp))
        return false;

    for (XmrTransaction& tx: txs_tmp)
    {
        // first we check if txs stored in db are already spendable
        // it means if they are older than 10 blocks. If  yes,
        // we mark them as spendable, as we assumet that blocks
        // older than 10 blocks are permanent, i.e, they wont get
        // orphaned.

        if (bool {tx.spendable} == false)
        {

            if (CurrentBlockchainStatus::is_tx_unlocked(tx.unlock_time, tx.height))
            {

                // this tx was before marked as unspendable, but now
                // it is spendable. Meaning, that its older than 10 blocks.
                // so mark it as spendable, so that its permanet.

                uint64_t no_row_updated = mark_tx_spendable(tx.id);

                if (no_row_updated != 1)
                {
//                    throw runtime_error("no_row_updated != 1 "
//                                                "due to "
//                                                "xmr_accounts->mark_tx_spendable(tx.id)");

                    cerr << "no_row_updated != 1 due to  xmr_accounts->mark_tx_spendable(tx.id)\n";
                    return false;
                }

                tx.spendable = true;
            }
            else
            {
                // tx was marked as non-spendable, i.e., younger than 10 blocks
                // so we still are going to use this txs, but we need to double
                // check if its still valid, i.e., it's block did not get orphaned.
                // we do this by checking if txs still exists in the blockchain
                // and if its blockchain_tx_id is same as what we have in our mysql.

                uint64_t blockchain_tx_id {0};

                CurrentBlockchainStatus::tx_exist(tx.hash, blockchain_tx_id);

                if (blockchain_tx_id != tx.blockchain_tx_id)
                {
                    // tx does not exist in blockchain, or its blockchain_id changed
                    // for example, it was orhpaned, and then readded.

                    uint64_t no_row_updated = delete_tx(tx.id);

                    if (no_row_updated != 1)
                    {
//                        throw runtime_error("no_row_updated != 1 "
//                                                    "due to "
//                                                    "xmr_accounts->delete_tx(tx.id)");
                        cerr << "no_row_updated != 1 due to  xmr_accounts->delete_tx(tx.id)\n";
                        return false;
                    }

                    // because txs does not exist in blockchain anymore,
                    // we assume its back to mempool, and it will be rescanned
                    // by tx search thread once added again to some block.

                    continue;

                }


                // set unlock_time field so that frontend displies it
                // as a locked tx, if unlock_time is zero.
                // coinbtase txs have this set already. regular tx
                // have unlock_time set to zero by default, but they cant
                // be spent anyway.

                if (tx.unlock_time == 0)
                    tx.unlock_time = tx.height + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE;

            } // else

        } // if (bool {tx.spendable} == false)

        txs.push_back(tx);

    } //for (XmrTransaction& tx: txs_tmp)

    return true;
}



bool
MySqlAccounts::select_outputs(const uint64_t& account_id, vector<XmrOutput>& outs)
{
    return mysql_out->select(account_id, outs);
}

bool
MySqlAccounts::select_output_with_id(const uint64_t& out_id, XmrOutput& out)
{
    return mysql_out->select(out_id, out);
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
MySqlAccounts::select_inputs_for_tx(const uint64_t& tx_id, vector<XmrInput>& ins)
{
    return mysql_in->select_for_tx(tx_id, ins);
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
MySqlAccounts::mark_tx_spendable(const uint64_t& tx_id_no)
{
    return mysql_tx->mark_spendable(tx_id_no);
}

uint64_t
MySqlAccounts::delete_tx(const uint64_t& tx_id_no)
{
    return mysql_tx->delete_tx(tx_id_no);
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
        return false;

    if (payments.empty())
        return false;

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

        return sr.rows() == 1;
    }
    catch (mysqlpp::Exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
        //return false;
    }
    catch (std::exception& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }

    return false;
}



/**
 * A completely different way to tackle this,
 * if your program doesn’t block forever waiting on I/O while idle,
 * is to periodically call Connection::ping(). [12]
 * This sends the smallest possible amount of data to the database server,
 * which will reset its idle timer and cause it to respond, so ping() returns true.
 * If it returns false instead, you know you need to reconnect to the server.
 * Periodic pinging is easiest to do if your program uses asynchronous I/O,
 * threads, or some kind of event loop to ensure that you can call
 * something periodically even while the rest of the program has nothing to do.
 *
 * from: https://tangentsoft.net/mysql++/doc/html/userman/tutorial.html#connopts
 */
void
MySqlAccounts::launch_mysql_pinging_thread()
{
    // lambda can only capture local variables, so we make
    // copy of the connection's shared pointer
    shared_ptr<MySqlConnector> conn_ptr = conn;

    std::thread ping_thread ([conn_ptr]()
     {
         while (true)
         {
             std::this_thread::sleep_for(chrono::seconds(7200)); // 2 hours

             if (!conn_ptr->ping())
             {
                 cerr << "Pinging mysql failed. stoping mysql pinging thread. \n";
                 break;
             }

             cout << "Mysql ping successful. \n" ;
         }
     });

    // run this in the background forever
    // we dont need to wait for it to finish
    ping_thread.detach();
}

shared_ptr<MySqlConnector>
MySqlAccounts::get_connection()
{
    return conn;
}



}