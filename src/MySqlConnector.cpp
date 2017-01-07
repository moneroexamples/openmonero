//
// Created by mwo on 7/01/17.
//

#include "MySqlConnector.h"

//#include "ssqlses.h"

#include <mysql++/mysql++.h>
#include <mysql++/ssqls.h>


#include <iostream>
#include <memory>


namespace xmreg {


string MySqlConnector::url;
string MySqlConnector::username;
string MySqlConnector::password;
string MySqlConnector::dbname;

MySqlConnector::MySqlConnector() {
    if (conn.connected())
        return;

    if (!conn.connect(dbname.c_str(), url.c_str(),
                      username.c_str(), password.c_str())) {
        cerr << "Connection to Mysql failed!" << endl;
        return;
    }
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

MySqlConnector::~MySqlConnector()
{
    conn.disconnect();
};

}