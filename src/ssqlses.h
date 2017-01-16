//
// Created by mwo on 14/12/16.
//

#ifndef RESTBED_XMR_SSQLSES_H
#define RESTBED_XMR_SSQLSES_H

#include "../ext/json.hpp"

#include <mysql++/mysql++.h>
#include <mysql++/ssqls.h>

namespace xmreg
{


using namespace std;
using namespace nlohmann;
using namespace mysqlpp;

sql_create_7(Accounts, 1, 2,
             sql_bigint_unsigned, id,
             sql_varchar        , address,
             sql_bigint_unsigned, total_received,
             sql_bigint_unsigned, scanned_block_height,
             sql_bigint_unsigned, start_height,
             sql_timestamp      , created,
             sql_timestamp      , modified);


struct XmrAccount : public Accounts
{

    static constexpr const char* SELECT_STMT = R"(
        SELECT * FROM `Accounts` WHERE `address` = (%0q)
    )";

    static constexpr const char* SELECT_STMT2 = R"(
        SELECT * FROM `Accounts` WHERE `id` = (%0q)
    )";

    static constexpr const char* INSERT_STMT = R"(
        INSERT INTO `Accounts` (`address`, `start_height`, `scanned_block_height`) VALUES (%0q, %1q , %2q);
    )";


    using Accounts::Accounts;

    // viewkey is not stored in mysql db or anywhere
    // so need to be populated when user logs in.
    string viewkey;

    json
    to_json() const;


    friend std::ostream& operator<< (std::ostream& stream, const XmrAccount& acc);

};

sql_create_13(Transactions, 1, 2,
              sql_bigint_unsigned, id,
              sql_varchar        , hash,
              sql_varchar        , prefix_hash,
              sql_bigint_unsigned, account_id,
              sql_bigint_unsigned, total_received,
              sql_bigint_unsigned, total_sent,
              sql_bigint_unsigned, unlock_time,
              sql_bigint_unsigned, height,
              sql_bool           , coinbase,
              sql_bool           , spendable,
              sql_varchar        , payment_id,
              sql_bigint_unsigned, mixin,
              sql_timestamp      , timestamp);


struct XmrTransaction : public Transactions
{

    static constexpr const char* SELECT_STMT = R"(
        SELECT * FROM `Transactions` WHERE `account_id` = (%0q)
    )";

    static constexpr const char* SELECT_STMT2 = R"(
        SELECT * FROM `Transactions` WHERE `id` = (%0q)
    )";

    static constexpr const char* EXIST_STMT = R"(
        SELECT * FROM `Transactions` WHERE `account_id` = (%0q) AND `hash` = (%1q)
    )";

    static constexpr const char* INSERT_STMT = R"(
        INSERT IGNORE INTO `Transactions` (`hash`, `prefix_hash`, `account_id`,
                                           `total_received`, `total_sent`, `unlock_time`,
                                           `height`, `coinbase`, `spendable`,
                                           `payment_id`, `mixin`, `timestamp`)
                                VALUES (%0q, %1q, %2q,
                                        %3q, %4q, %5q,
                                        %6q, %7q, %8q,
                                        %9q, %10q, %11q);
    )";

    static constexpr const char* SUM_XMR_RECIEVED = R"(
        SELECT SUM(`total_received`) AS total_received
               FROM `Transactions`
               WHERE `account_id` = %0q
               GROUP BY `account_id`
    )";




    using Transactions::Transactions;

    json
    to_json() const;

    static DateTime
    timestamp_to_DateTime(time_t timestamp);

    friend std::ostream& operator<< (std::ostream& stream, const XmrTransaction& acc);

};

sql_create_10(Outputs, 1, 3,
              sql_bigint_unsigned, id,
              sql_bigint_unsigned, account_id,
              sql_bigint_unsigned, tx_id,
              sql_varchar        , out_pub_key,
              sql_varchar        , tx_pub_key,
              sql_bigint_unsigned, amount,
              sql_bigint_unsigned, global_index,
              sql_bigint_unsigned, out_index,
              sql_bigint_unsigned, mixin,
              sql_timestamp      , timestamp);

struct XmrOutput : public Outputs
{

    static constexpr const char* SELECT_STMT = R"(
      SELECT * FROM `Outputs` WHERE `account_id` = (%0q)
    )";

    static constexpr const char* SELECT_STMT2 = R"(
      SELECT * FROM `Outputs` WHERE `tx_id` = (%0q)
    )";

    static constexpr const char* EXIST_STMT = R"(
      SELECT * FROM `Outputs` WHERE `out_pub_key` = (%0q)
    )";

    static constexpr const char* INSERT_STMT = R"(
      INSERT IGNORE INTO `Outputs` (`account_id`, `tx_id`, `out_pub_key`, `tx_pub_key`,
                                     `amount`, `global_index`, `out_index`, `mixin`, `timestamp`)
                            VALUES (%0q, %1q, %2q, %3q,
                                    %4q, %5q, %6q, %7q, %8q);
    )";



    using Outputs::Outputs;

    json
    to_json() const;


    friend std::ostream& operator<< (std::ostream& stream, const XmrOutput& out);

};


sql_create_7(Inputs, 1, 4,
             sql_bigint_unsigned, id,
             sql_bigint_unsigned, account_id,
             sql_bigint_unsigned, tx_id,
             sql_bigint_unsigned, output_id,
             sql_varchar        , key_image,
             sql_bigint_unsigned, amount,
             sql_timestamp      , timestamp);


struct XmrInput : public Inputs
{

    static constexpr const char* SELECT_STMT = R"(
     SELECT * FROM `Inputs` WHERE `account_id` = (%0q)
    )";

    static constexpr const char* SELECT_STMT2 = R"(
     SELECT * FROM `Inputs` WHERE `tx_id` = (%0q)
    )";

    static constexpr const char* SELECT_STMT3 = R"(
     SELECT * FROM `Inputs` WHERE `output_id` = (%0q)
    )";

    static constexpr const char* INSERT_STMT = R"(
      INSERT IGNORE INTO `Inputs` (`account_id`, `tx_id`, `output_id`,
                                `key_image`, `amount` , `timestamp`)
                        VALUES (%0q, %1q, %2q,
                                %3q, %4q, %5q);
    )";

    using Inputs::Inputs;

    json
    to_json() const;

    friend std::ostream& operator<< (std::ostream& stream, const XmrInput& out);

};

sql_create_9(Payments, 1, 2,
             sql_bigint_unsigned, id,
             sql_varchar        , address,
             sql_varchar        , payment_id,
             sql_varchar        , tx_hash,
             sql_boolean        , request_fulfilled,
             sql_varchar        , payment_address,
             sql_bigint_unsigned, import_fee,
             sql_timestamp      , created,
             sql_timestamp      , modified);


struct XmrPayment : public Payments
{

    static constexpr const char* SELECT_STMT = R"(
      SELECT * FROM `Payments` WHERE `address` = (%0q)
    )";

    static constexpr const char* SELECT_STMT2 = R"(
       SELECT * FROM `Payments` WHERE `payment_id` = (%0q)
    )";


    static constexpr const char* INSERT_STMT = R"(
       INSERT IGNORE INTO `Payments` (`address`, `payment_id`, `tx_hash`,
                                 `request_fulfilled`, `payment_address`, `import_fee`)
                    VALUES (%0q, %1q, %2q,
                            %3q, %4q, %5q);
    )";



    using Payments::Payments;

    json
    to_json() const;

    friend std::ostream& operator<< (std::ostream& stream, const XmrPayment& out);

};

sql_create_10(TransactionsWithOutsAndIns, 1, 2,
              sql_bigint_unsigned, tx_id,
              sql_bigint_unsigned, account_id,
              sql_varchar        , out_pub_key,
              sql_bigint_unsigned, amount,
              sql_bigint_unsigned, out_index,
              sql_bigint_unsigned, global_index,
              sql_varchar        , tx_pub_key,
              sql_timestamp      , timestamp,
              sql_varchar_null   , key_image,
              sql_bigint_unsigned, mixin);

struct XmrTransactionWithOutsAndIns : public TransactionsWithOutsAndIns
{

    static constexpr const char* SELECT_STMT = R"(
       SELECT * FROM `TransactionsWithOutsAndIns` WHERE `account_id` = (%0q)
    )";

    static constexpr const char* SELECT_STMT2 = R"(
       SELECT * FROM `TransactionsWithOutsAndIns` WHERE `tx_id` = (%0q)
    )";


    using TransactionsWithOutsAndIns::TransactionsWithOutsAndIns;

    json
    to_json() const;

    json
    spent_output() const;

    string
    key_image_to_string() const;

    friend std::ostream& operator<< (std::ostream& stream,
                                     const XmrTransactionWithOutsAndIns& out);

};





}


#endif //RESTBED_XMR_SSQLSES_H
