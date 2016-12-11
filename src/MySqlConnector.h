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

    template <typename T>
    using mysql_unqiue_ptr =  unique_ptr<T, MySqlDeleter<T>>;

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
sql_create_11(AccountRow, 1, 0,
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

struct Account : public AccountRow
{
    using AccountRow::AccountRow;
};



class MySqlAccounts: public MySqlConnector
{
    static constexpr const char* TABLE_NAME = "Accounts";

    static constexpr const char* SELECT_STMT = R"(
        SELECT * FROM `Accounts` WHERE `address` = (%0q)
    )";

    static constexpr const char* INSERT_STMT = R"(
        INSERT INTO `Accounts`  (`address`) VALUES (%0q)
    )";


public:
    MySqlAccounts(): MySqlConnector() {}


    bool
    select_account(const string& address, Account& account)
    {

        Query query = conn.query(SELECT_STMT);
        query.parse();

        try
        {
            vector<Account> res;
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

//    bool
//    create_account(const string& address)
//    {
//        mysql_unqiue_ptr<PreparedStatement> prep_stmt {con->prepareStatement(SELECT_STMT)};
//
//        prep_stmt->setString(1, address);
//
//        try
//        {
//            prep_stmt->execute();
//        }
//        catch (SQLException& e)
//        {
//            MYSQL_EXCEPTION_MSG(e);
//            return false;
//        }
//
//        return true;
//    }
//
//    bool
//    update_account(
//            const string& address,
//            const uint64_t& total_recieved = 0,
//            const uint64_t& total_sent = 0,
//            const uint64_t& scanned_height = 0,
//            const uint64_t& scanned_block_height = 0,
//            const uint64_t& start_height = 0,
//            const uint64_t& transaction_height = 0,
//            const uint64_t& blockchain_height = 0
//    )
//    {
//        mysql_unqiue_ptr<PreparedStatement> prep_stmt(con->prepareStatement(INSERT_STMT));
//
//        prep_stmt->setString(1, address);
//        prep_stmt->setUInt64(2, total_recieved);
//        prep_stmt->setString(3, "a");
//        prep_stmt->execute();
//
//        return false;
//    }

};


}


#endif //RESTBED_XMR_MYSQLCONNECTOR_H
