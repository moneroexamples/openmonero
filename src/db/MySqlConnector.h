//
// Created by mwo on 9/12/16.
//

#ifndef RESTBED_XMR_MYSQLCONNECTOR_H
#define RESTBED_XMR_MYSQLCONNECTOR_H

//#include "tools.h"

#include <mysql++/mysql++.h>
#include <mysql++/ssqls.h>


#include <iostream>

namespace xmreg
{

using namespace mysqlpp;
using namespace std;

#define MYSQL_EXCEPTION_MSG(sql_excetption) cerr << "# ERR: SQLException in " << __FILE__ \
         << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl \
         << "# ERR: " << sql_excetption.what() \
         << endl;


/*
 * This is that creates connections
 * our mysql database.
 *
 * Each thread has its own connection.
 * This way when something breaks in one thread,
 * other threads can still operate on the mysql.
 *
 */
class MySqlConnector
{

    Connection conn;

public:

    static string url;
    static size_t port;
    static string username;
    static string password;
    static string dbname;

    MySqlConnector();

    MySqlConnector(Option* _option);

    // dont want any copies of connection object.
    // pass it around through shared or unique pointer
    MySqlConnector (const MySqlConnector&) = delete;
    MySqlConnector& operator= (const MySqlConnector&) = delete;

    Query
    query(const char* qstr = 0);

    Query
    query(const std::string& qstr);

    virtual bool
    connect();

    bool
    ping();

    Connection&
    get_connection();

    // this throws exception if not connected
    inline void
    check_if_connected()
    {
        if (!conn.connected())
            throw std::runtime_error("No connection to the mysqldb");
    }

    virtual ~MySqlConnector();

protected:

    void _init();

};


}


#endif //RESTBED_XMR_MYSQLCONNECTOR_H
