//
// Created by mwo on 9/12/16.
//

#ifndef RESTBED_XMR_MYSQLCONNECTOR_H
#define RESTBED_XMR_MYSQLCONNECTOR_H

#include <iostream>
#include <memory>



#include "mysql_connection.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>



namespace xmreg
{

using namespace sql;
using namespace std;


#define MYSQL_EXCEPTION_MSG(sql_excetption) cerr << "# ERR: SQLException in " << __FILE__ \
         << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl \
         << "# ERR: " << sql_excetption.what() \
         << " (MySQL error code: " <<sql_excetption.getErrorCode() \
         << ", SQLState: " << sql_excetption.getSQLState() << " )" \
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
    Driver* driver;

    unique_ptr<Connection> con;

public:
    MySqlConnector(
        string _url      = "tcp://127.0.0.1:3306",
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
        Driver* driver = get_driver_instance();
        con = unique_ptr<Connection>(driver->connect(url, username, password));
        con->setSchema(dbname);
    }

};


class MySqlAccounts: public MySqlConnector
{
    static constexpr const char* TABLE_NAME = "Accounts";

    static constexpr const char* INSERT_STMT = R"(
        INSERT INTO `Accounts`  (`address`) VALUES (?)
    )";

    static constexpr const char* SELECT_STMT = R"(
        SELECT * FROM `Accounts` WHERE `address` = ?
    )";

public:
    MySqlAccounts(): MySqlConnector() {}


    bool
    select_account(const string& address)
    {
        mysql_unqiue_ptr<PreparedStatement> prep_stmt {con->prepareStatement(SELECT_STMT)};

        prep_stmt->setString(1, address);

        try
        {
            unique_ptr<ResultSet> res {prep_stmt->executeQuery()};

            cout << res->getRow() << endl;

        }
        catch (SQLException& e)
        {
            MYSQL_EXCEPTION_MSG(e);
            return false;
        }

        return true;
    }

    bool
    create_account(const string& address)
    {
        mysql_unqiue_ptr<PreparedStatement> prep_stmt {con->prepareStatement(SELECT_STMT)};

        prep_stmt->setString(1, address);

        try
        {
            prep_stmt->execute();
        }
        catch (SQLException& e)
        {
            MYSQL_EXCEPTION_MSG(e);
            return false;
        }

        return true;
    }

    bool
    update_account(
            const string& address,
            const uint64_t& total_recieved = 0,
            const uint64_t& total_sent = 0,
            const uint64_t& scanned_height = 0,
            const uint64_t& scanned_block_height = 0,
            const uint64_t& start_height = 0,
            const uint64_t& transaction_height = 0,
            const uint64_t& blockchain_height = 0
    )
    {
        mysql_unqiue_ptr<PreparedStatement> prep_stmt(con->prepareStatement(INSERT_STMT));

        prep_stmt->setString(1, address);
        prep_stmt->setUInt64(2, total_recieved);
        prep_stmt->setString(3, "a");
        prep_stmt->execute();

        return false;
    }

};


}


#endif //RESTBED_XMR_MYSQLCONNECTOR_H
