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

    Query
    query(const char* qstr = 0);

    Query
    query(const std::string& qstr);

    bool
    connect();

    bool
    ping();

    Connection&
    get_connection();

    virtual ~MySqlConnector();
};


}


#endif //RESTBED_XMR_MYSQLCONNECTOR_H
