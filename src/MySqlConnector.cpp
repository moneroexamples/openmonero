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
string MySqlConnector::username;
string MySqlConnector::password;
string MySqlConnector::dbname;

MySqlConnector::MySqlConnector()
{
    if (!connect())
    {
        cerr << "Connection to Mysql failed!" << endl;
        return;
    }
}

bool
MySqlConnector::connect()
{
    if (conn.connected())
        return true;

    if (!conn.connect(dbname.c_str(), url.c_str(),
                      username.c_str(), password.c_str())) {
        cerr << "Connection to Mysql failed!" << endl;
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

}