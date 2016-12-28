//
// Created by mwo on 16/12/16.
//

#ifndef RESTBED_XMR_MYSQLACCOUNTS_H
#define RESTBED_XMR_MYSQLACCOUNTS_H


#include "tools.h"
#include "ssqlses.h"
#include "MySqlConnector.h"

#include <mysql++/mysql++.h>
#include <mysql++/ssqls.h>


#include <iostream>
#include <memory>


namespace xmreg
{

using namespace mysqlpp;
using namespace std;
using namespace nlohmann;


class MysqlTransactionWithOutsAndIns
{

    shared_ptr<MySqlConnector> conn;

public:

    MysqlTransactionWithOutsAndIns(shared_ptr<MySqlConnector> _conn) : conn{_conn} {}

    bool
    select(const uint64_t &address_id, vector<XmrTransactionWithOutsAndIns>& txs) {

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
    select_for_tx(const uint64_t &tx_id, vector<XmrTransactionWithOutsAndIns>& txs) {

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
};


class MysqlInputs
{

    shared_ptr<MySqlConnector> conn;

public:

    MysqlInputs(shared_ptr<MySqlConnector> _conn): conn {_conn}
    {}

    bool
    select(const uint64_t& address_id, vector<XmrInput>& ins)
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
    select_for_tx(const uint64_t& address_id, vector<XmrInput>& ins)
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


    uint64_t
    insert(const XmrInput& in_data)
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

};



class MysqlOutpus
{

    shared_ptr<MySqlConnector> conn;

public:

    MysqlOutpus(shared_ptr<MySqlConnector> _conn): conn {_conn}
    {}

    bool
    select(const uint64_t& address_id, vector<XmrOutput>& outs)
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
    exist(const string& output_public_key_str, XmrOutput& out)
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
    insert(const XmrOutput& out_data)
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

};



class MysqlTransactions
{

    shared_ptr<MySqlConnector> conn;

public:

    MysqlTransactions(shared_ptr<MySqlConnector> _conn): conn {_conn}
    {}

    bool
    select(const uint64_t& address_id, vector<XmrTransaction>& txs)
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
    insert(const XmrTransaction& tx_data)
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
    exist(const string& tx_hash_str, XmrTransaction& tx)
    {

        Query query = conn->query(XmrTransaction::EXIST_STMT);
        query.parse();

        try
        {

            vector<XmrTransaction> outs;

            query.storein(outs, tx_hash_str);

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
    get_total_recieved(const uint64_t& account_id)
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


};


class MySqlAccounts
{

    shared_ptr<MySqlConnector> conn;

    shared_ptr<MysqlTransactions> mysql_tx;

    shared_ptr<MysqlOutpus> mysql_out;

    shared_ptr<MysqlInputs> mysql_in;

    shared_ptr<MysqlTransactionWithOutsAndIns> mysql_tx_inout;


public:


    MySqlAccounts()
    {
        // create connection to the mysql
        conn            = make_shared<MySqlConnector>();

        // use same connection with working with other tables
        mysql_tx        = make_shared<MysqlTransactions>(conn);
        mysql_out       = make_shared<MysqlOutpus>(conn);
        mysql_in        = make_shared<MysqlInputs>(conn);
        mysql_tx_inout  = make_shared<MysqlTransactionWithOutsAndIns>(conn);
    }


    bool
    select(const string& address, XmrAccount& account)
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
    select(const int64_t& acc_id, XmrAccount& account)
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
    insert(const string& address, const uint64_t& current_blkchain_height = 0)
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
    insert_tx(const XmrTransaction& tx_data)
    {
        return mysql_tx->insert(tx_data);
    }

    uint64_t
    insert_output(const XmrOutput& tx_out)
    {
        return mysql_out->insert(tx_out);
    }

    uint64_t
    insert_input(const XmrInput& tx_in)
    {
        return mysql_in->insert(tx_in);
    }

    bool
    select_txs(const string& xmr_address, vector<XmrTransaction>& txs)
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
    select_txs(const uint64_t& account_id, vector<XmrTransaction>& txs)
    {
        return mysql_tx->select(account_id, txs);
    }


    bool
    select_txs_with_inputs_and_outputs(const uint64_t& account_id,
                                       vector<XmrTransactionWithOutsAndIns>& txs)
    {
        return mysql_tx_inout->select(account_id, txs);
    }


    bool
    select_outputs(const uint64_t& account_id, vector<XmrOutput>& outs)
    {
        return mysql_out->select(account_id, outs);
    }

    bool
    select_inputs(const uint64_t& account_id, vector<XmrInput>& ins)
    {
        return mysql_in->select(account_id, ins);
    }

    bool
    select_inputs_for_tx(const uint64_t& tx_id, vector<XmrTransactionWithOutsAndIns>& ins)
    {
        return mysql_tx_inout->select_for_tx(tx_id, ins);
    }

    bool
    output_exists(const string& output_public_key_str, XmrOutput& out)
    {
        return mysql_out->exist(output_public_key_str, out);
    }

    bool
    tx_exists(const string& tx_hash_str, XmrTransaction& tx)
    {
        return mysql_tx->exist(tx_hash_str, tx);
    }


    uint64_t
    get_total_recieved(const uint64_t& account_id)
    {
        return mysql_tx->get_total_recieved(account_id);
    }


    bool
    update(XmrAccount& acc_orginal, XmrAccount& acc_new)
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

};


}


#endif //RESTBED_XMR_MYSQLACCOUNTS_H
