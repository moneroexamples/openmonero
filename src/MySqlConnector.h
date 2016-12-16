//
// Created by mwo on 9/12/16.
//

#ifndef RESTBED_XMR_MYSQLCONNECTOR_H
#define RESTBED_XMR_MYSQLCONNECTOR_H

#include "tools.h"
#include "ssqlses.h"

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
    static string username;
    static string password;
    static string dbname;

    MySqlConnector()
    {
        if (conn.connected())
            return;

        if (!conn.connect(dbname.c_str(), url.c_str(),
                         username.c_str(), password.c_str()))
        {
            cerr << "Connection to Mysql failed!" << endl;
            return;
        }
    }

    Query
    query(const char* qstr = 0)
    {
        return conn.query(qstr);
    }

    Query
    query(const std::string& qstr)
    {
        return conn.query(qstr);
    }

    virtual ~MySqlConnector()
    {
        conn.disconnect();
    };
};

string MySqlConnector::url;
string MySqlConnector::username;
string MySqlConnector::password;
string MySqlConnector::dbname;








}


#endif //RESTBED_XMR_MYSQLCONNECTOR_H
