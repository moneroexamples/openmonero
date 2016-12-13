//
// Created by mwo on 9/12/16.
//

#ifndef RESTBED_XMR_MYSQLCONNECTOR_H
#define RESTBED_XMR_MYSQLCONNECTOR_H

#include "tools.h"

#include <mysql++/mysql++.h>
#include <mysql++/ssqls.h>


#include <iostream>
#include <memory>


namespace xmreg
{

using namespace mysqlpp;
using namespace std;
using namespace nlohmann;

#define MYSQL_EXCEPTION_MSG(sql_excetption) cerr << "# ERR: SQLException in " << __FILE__ \
         << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl \
         << "# ERR: " << sql_excetption.what() \
         << endl;


class MySqlConnector
{
    string url;
    string username;
    string password;
    string dbname;

protected:

    Connection conn;

public:
    MySqlConnector(
        string _url      = "127.0.0.1",
        string _username = "root",
        string _password = "root",
        string _dbname   = "yourmonero"
    ):
        url      {_url},
        username {_username},
        password {_password},
        dbname   {_dbname}
    {
        connect();
    }

    void
    connect()
    {
        if (conn.connect(dbname.c_str(), url.c_str(),
                         username.c_str(), password.c_str()))
        {
            cout << "Connection to Mysql successful" << endl;
        }
    }

    virtual ~MySqlConnector() {};

};



// to see what it does can run preprecoess on this file
// g++ -I/usr/include/mysql -E ~/restbed-xmr/src/MySqlConnector.h > /tmp/out.h
sql_create_11(Accounts, 1, 2,
             sql_int_unsigned   , id,
             sql_varchar        , address,
             sql_bigint_unsigned, total_received,
             sql_bigint_unsigned, scanned_height,
             sql_bigint_unsigned, scanned_block_height,
             sql_bigint_unsigned, start_height,
             sql_bigint_unsigned, transaction_height,
             sql_bigint_unsigned, blockchain_height,
             sql_bigint_unsigned, total_sent,
             sql_timestamp      , created,
             sql_timestamp      , modified);



struct XmrAccount : public Accounts
{
    using Accounts::Accounts;

    // viewkey is not stored in mysql db or anywhere
    // so need to be populated when user logs in.
    string viewkey;

    json
    to_json() const
    {
        json j {{"id"                  , id},
                {"address"             , address},
                {"viewkey"             , viewkey},
                {"total_received"      , total_received},
                {"total_sent"          , total_sent},
                {"scanned_block_height", scanned_block_height},
                {"blockchain_height"   , blockchain_height}
        };

        return j;
    }


    friend std::ostream& operator<< (std::ostream& stream, const XmrAccount& acc);

};

ostream& operator<< (std::ostream& os, const XmrAccount& acc) {
    os << "XmrAccount: " << acc.to_json().dump() << '\n';
    return os;
};

class MySqlAccounts: public MySqlConnector
{
    static constexpr const char* TABLE_NAME = "Accounts";

    static constexpr const char* SELECT_STMT = R"(
        SELECT * FROM `Accounts` WHERE `address` = (%0q)
    )";

    static constexpr const char* SELECT_STMT2 = R"(
        SELECT * FROM `Accounts` WHERE `id` = (%0q)
    )";

    static constexpr const char* INSERT_STMT = R"(
        INSERT INTO `Accounts` (`address`, `scanned_block_height`) VALUES (%0q, %1q);
    )";




public:
    MySqlAccounts(): MySqlConnector() {}


    bool
    select(const string& address, XmrAccount& account)
    {

        static shared_ptr<Query> query;

        if (!query)
        {
            Query q = conn.query(SELECT_STMT);
            q.parse();
            query = shared_ptr<Query>(new Query(q));
        }

//        Query query = conn.query(SELECT_STMT);
//        query.parse();

        try
        {
            vector<XmrAccount> res;
            query->storein(res, address);

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

        static shared_ptr<Query> query;

        if (!query)
        {
            Query q = conn.query(SELECT_STMT2);
            q.parse();
            query = shared_ptr<Query>(new Query(q));
        }

//        Query query = conn.query(SELECT_STMT2);
//        query.parse();

        try
        {
            vector<XmrAccount> res;
            query->storein(res, acc_id);

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
    create(const string& address, const uint64_t& scanned_block_height = 0)
    {

        static shared_ptr<Query> query;

        if (!query)
        {
            Query q = conn.query(INSERT_STMT);
            q.parse();
            query = shared_ptr<Query>(new Query(q));
        }


//        Query query = conn.query(INSERT_STMT);
//        query.parse();

        cout << query << endl;

        try
        {
            SimpleResult sr = query->execute(address, scanned_block_height);

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
    update(XmrAccount& acc_orginal, XmrAccount& acc_new)
    {

        Query query = conn.query();

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


#endif //RESTBED_XMR_MYSQLCONNECTOR_H
