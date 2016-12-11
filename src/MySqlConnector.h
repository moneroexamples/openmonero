//
// Created by mwo on 9/12/16.
//

#ifndef RESTBED_XMR_MYSQLCONNECTOR_H
#define RESTBED_XMR_MYSQLCONNECTOR_H


#include <mysql++/mysql++.h>
#include <mysql++/ssqls.h>


#include <iostream>
#include <memory>


namespace xmreg
{

using namespace mysqlpp;
using namespace std;


#define MYSQL_EXCEPTION_MSG(sql_excetption) cerr << "# ERR: SQLException in " << __FILE__ \
         << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl \
         << "# ERR: " << sql_excetption.what() \
         << endl;

/**
 * Custom deleter for some MySql object
 * as they are often virtual thus cant use unique_ptr
 * @tparam T
 */
template <typename T>
struct MySqlDeleter
{
    void
    operator()(T *p)
    {
        delete p;
    }
};


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
};



class MySqlAccounts: public MySqlConnector
{
    static constexpr const char* TABLE_NAME = "Accounts";

    static constexpr const char* SELECT_STMT = R"(
        SELECT * FROM `Accounts` WHERE `address` = (%0q)
    )";

    static constexpr const char* INSERT_STMT = R"(
        INSERT INTO `Accounts` (`address`) VALUES (%0q);
    )";


public:
    MySqlAccounts(): MySqlConnector() {}


    bool
    select(const string& address, XmrAccount& account)
    {

        Query query = conn.query(SELECT_STMT);
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

        return false;
    }

    bool
    create(const string& address)
    {
        Query query = conn.query(INSERT_STMT);
        query.parse();

        try
        {
            SimpleResult sr = query.execute(address);

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
