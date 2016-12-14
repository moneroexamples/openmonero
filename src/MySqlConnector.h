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
 * This is singleton class that connects
 * and holds connection to our mysql.
 *
 * Only one instance of it can exist.
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

    static MySqlConnector& getInstance()
    {
        static MySqlConnector  instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
private:
    MySqlConnector()
    {
        if (conn.connected())
            return;

        if (conn.connect(dbname.c_str(), url.c_str(),
                         username.c_str(), password.c_str()))
        {
            cout << "Connection to Mysql successful" << endl;
        }
    }

public:
    MySqlConnector(MySqlConnector const&)  = delete;
    void operator=(MySqlConnector const&)  = delete;

//    Connection*
//    get_connection()
//    {
//        return &conn;
//    }

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

class MysqlTransactions
{

public:

    bool
    select(const string& address)
    {

        static shared_ptr<Query> query;

        if (!query)
        {
            Query q = MySqlConnector::getInstance().query(
                    XmrTransaction::SELECT_STMT);
            q.parse();
            query = shared_ptr<Query>(new Query(q));
        }


        try
        {
            vector<XmrTransaction> res;
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

};



class MySqlAccounts
{

public:


    bool
    select(const string& address, XmrAccount& account)
    {

        static shared_ptr<Query> query;

        if (!query)
        {
            Query q = MySqlConnector::getInstance().query(XmrAccount::SELECT_STMT);
            q.parse();
            query = shared_ptr<Query>(new Query(q));
        }

//        Query query = conn.query(XmrAccount::SELECT_STMT);
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
            Query q = MySqlConnector::getInstance().query(XmrAccount::SELECT_STMT2);
            q.parse();
            query = shared_ptr<Query>(new Query(q));
        }

//        Query query = conn.query(XmrAccount::SELECT_STMT2);
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
            Query q = MySqlConnector::getInstance().query(XmrAccount::INSERT_STMT);
            q.parse();
            query = shared_ptr<Query>(new Query(q));
        }


//        Query query = conn.query(XmrAccount::INSERT_STMT);
//        query.parse();

       // cout << query << endl;

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

        Query query = MySqlConnector::getInstance().query();

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
