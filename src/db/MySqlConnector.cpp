//
// Created by mwo on 7/01/17.
//

#include "MySqlConnector.h"

#include <mysql++/mysql++.h>
#include <mysql++/ssqls.h>


#include <iostream>
#include <memory>


namespace xmreg {


string MySqlConnector::url;
size_t MySqlConnector::port;
string MySqlConnector::username;
string MySqlConnector::password;
string MySqlConnector::dbname;

MySqlConnector::MySqlConnector()
{
    _init();
}

MySqlConnector::MySqlConnector(Option* _option)
{
    conn.set_option(_option);
    _init();
}

bool
MySqlConnector::connect()
{
    if (conn.connected())
        return true;

    try
    {
        conn.connect(dbname.c_str(),
                     url.c_str(),
                     username.c_str(),
                     password.c_str(),
                     port);
    }
    catch (mysqlpp::ConnectionFailed const& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        return false;
    }

    return true;
}


bool
MySqlConnector::ping()
{
    return conn.ping();
}

Query
MySqlConnector::query(const char *qstr)
{
    return conn.query(qstr);
}

Query
MySqlConnector::query(const std::string &qstr)
{
    return conn.query(qstr);
}

Connection&
MySqlConnector::get_connection()
{
    return conn;
}

MySqlConnector::~MySqlConnector()
{
    conn.disconnect();
};

void
MySqlConnector::_init()
{
    if (!connect())
    {
        cerr << "Connection to Mysql failed!" << endl;
        throw std::runtime_error("Connection to Mysql failed!");
    }
}

}