//
// Created by mwo on 16/12/16.
//

#ifndef RESTBED_XMR_MYSQLACCOUNTS_H
#define RESTBED_XMR_MYSQLACCOUNTS_H

#include "tools.h"
#include "MySqlConnector.h"



#include <iostream>
#include <memory>



namespace xmreg
{

using namespace mysqlpp;
using namespace std;
using namespace nlohmann;


class XmrTransactionWithOutsAndIns;
class XmrInput;
class XmrOutput;
class XmrTransaction;
class XmrPayment;
class XmrAccount;
class TxSearch;
class Table;


class MysqlInputs
{

    shared_ptr<MySqlConnector> conn;

public:

    MysqlInputs(shared_ptr<MySqlConnector> _conn);

    bool
    select_for_out(const uint64_t& output_id, vector<XmrInput>& ins);
};



class MysqlOutpus
{

    shared_ptr<MySqlConnector> conn;

public:

    MysqlOutpus(shared_ptr<MySqlConnector> _conn);

    bool
    exist(const string& output_public_key_str, XmrOutput& out);
};



class MysqlTransactions
{

    shared_ptr<MySqlConnector> conn;

public:

    MysqlTransactions(shared_ptr<MySqlConnector> _conn);

    uint64_t
    mark_spendable(const uint64_t& tx_id_no);

    uint64_t
    mark_nonspendable(const uint64_t& tx_id_no);

    uint64_t
    delete_tx(const uint64_t& tx_id_no);

    bool
    exist(const uint64_t& account_id, const string& tx_hash_str, XmrTransaction& tx);

    uint64_t
    get_total_recieved(const uint64_t& account_id);
};

class MysqlPayments
{

    shared_ptr<MySqlConnector> conn;

public:

    MysqlPayments(shared_ptr<MySqlConnector> _conn);

    bool
    select_by_payment_id(const string& payment_id, vector<XmrPayment>& payments);

    bool
    update(XmrPayment& payment_orginal, XmrPayment& payment_new);
};

class TxSearch;

class MySqlAccounts
{

    shared_ptr<MySqlConnector> conn;

    shared_ptr<MysqlTransactions> mysql_tx;

    shared_ptr<MysqlOutpus> mysql_out;

    shared_ptr<MysqlInputs> mysql_in;

    shared_ptr<MysqlPayments> mysql_payment;


public:

    MySqlAccounts();

    MySqlAccounts(shared_ptr<MySqlConnector> _conn);

    bool
    select(const string& address, XmrAccount& account);

    template <typename T>
    uint64_t
    insert(const T& data_to_insert);

    template <typename T>
    uint64_t
    insert(const vector<T>& data_to_insert);

    /**
     *
     * @tparam T
     * @tparam query_no which query to use, for SELECT_STMT or SELECT_STMT2
     * @param account_id
     * @param selected_data
     * @return
     */
    template <typename T, size_t query_no = 1>
    bool
    select(uint64_t account_id, vector<T>& selected_data);

    template <typename T>
    bool
    select_for_tx(uint64_t tx_id, vector<T>& selected_data);

    template <typename T>
    bool
    select_by_primary_id(uint64_t id, T& selected_data);

    bool
    select_txs_for_account_spendability_check(const uint64_t& account_id,
                                              vector<XmrTransaction>& txs);

    bool
    select_inputs_for_out(const uint64_t& output_id, vector<XmrInput>& ins);

    bool
    output_exists(const string& output_public_key_str, XmrOutput& out);

    bool
    tx_exists(const uint64_t& account_id, const string& tx_hash_str, XmrTransaction& tx);

    uint64_t
    mark_tx_spendable(const uint64_t& tx_id_no);

    uint64_t
    mark_tx_nonspendable(const uint64_t& tx_id_no);

    uint64_t
    delete_tx(const uint64_t& tx_id_no);

    bool
    select_payment_by_id(const string& payment_id, vector<XmrPayment>& payments);

    bool
    update_payment(XmrPayment& payment_orginal, XmrPayment& payment_new);

    uint64_t
    get_total_recieved(const uint64_t& account_id);

    bool
    update(XmrAccount& acc_orginal, XmrAccount& acc_new);

    void
    launch_mysql_pinging_thread();

    shared_ptr<MySqlConnector>
    get_connection();

    /**
     * DONT use!!!
     *
     * Its only useful in unit tests when you know that nothing will insert
     * any row between calling this and using the returned id
     *
     * @tparam T
     * @param table_class
     * @return
     */
    template <typename T>
    uint64_t
    get_next_primary_id(T&& table_class)
    {
        static_assert(std::is_base_of<Table, std::decay_t<T>>::value, "given class is not Table");

        string sql {"SELECT `auto_increment` FROM INFORMATION_SCHEMA.TABLES WHERE table_name = '"};
        sql += table_class.table_name() + "' AND table_schema = '" +  MySqlConnector::dbname + "'";

        Query query = conn->query(sql);
        query.parse();

        try
        {
            StoreQueryResult  sr = query.store();

            if (!sr.empty())
                return sr[0][0];
        }
        catch (std::exception& e)
        {
            MYSQL_EXCEPTION_MSG(e);
        }

        return 0;
    }

private:
    void _init();
};


}


#endif //RESTBED_XMR_MYSQLACCOUNTS_H
