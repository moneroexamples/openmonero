//
// Created by mwo on 14/12/16.
//

#ifndef RESTBED_XMR_SSQLSES_H
#define RESTBED_XMR_SSQLSES_H


#include <mysql++/mysql++.h>
#include <mysql++/ssqls.h>

#include "../ext/json.hpp"

namespace xmreg
{

using namespace mysqlpp;
using namespace std;
using namespace nlohmann;



// to see what it does can run preprecoess on this file
// g++ -I/usr/include/mysql -E ~/restbed-xmr/src/MySqlConnector.h > /tmp/out.h
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

    XmrAccount():Accounts()
    {
        address = "";
        total_received = 0;
        scanned_block_height = 0;
        start_height = 0;
    }

    // viewkey is not stored in mysql db or anywhere
    // so need to be populated when user logs in.
    string viewkey;

    json
    to_json() const
    {
        json j {{"id"                  , id},
                {"address"             , address},
                {"viewkey"             , viewkey},
                {"total_received"      , total_received},
                {"scanned_block_height", scanned_block_height},
                {"start_height"        , start_height}
        };

        return j;
    }


    friend std::ostream& operator<< (std::ostream& stream, const XmrAccount& acc);

};

ostream& operator<< (std::ostream& os, const XmrAccount& acc) {
    os << "XmrAccount: " << acc.to_json().dump() << '\n';
    return os;
};


sql_create_12(Transactions, 1, 2,
              sql_bigint_unsigned, id,
              sql_varchar        , hash,
              sql_varchar        , prefix_hash,
              sql_bigint_unsigned, account_id,
              sql_bigint_unsigned, total_received,
              sql_bigint_unsigned, total_sent,
              sql_bigint_unsigned, unlock_time,
              sql_bigint_unsigned, height,
              sql_bool           , coinbase,
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
        SELECT * FROM `Transactions` WHERE `hash` = (%0q)
    )";

    static constexpr const char* INSERT_STMT = R"(
        INSERT IGNORE INTO `Transactions` (`hash`, `prefix_hash` ,
                                     `account_id`, `total_received`,
                                    `total_sent`, `unlock_time`, `height`,
                                    `coinbase`, `payment_id`, `mixin`,
                                    `timestamp`)
                                VALUES (%0q, %1q, %2q,
                                        %3q, %4q, %5q,
                                        %6q, %7q, %8q,
                                        %9q, %10q);
    )";

    static constexpr const char* SUM_XMR_RECIEVED = R"(
        SELECT SUM(`total_received`) AS total_received
               FROM `Transactions`
               WHERE `account_id` = %0q
               GROUP BY `account_id`
    )";




    using Transactions::Transactions;

    json
    to_json() const
    {
        json j {{"id"                  , id},
                {"hash"                , hash},
                {"prefix_hash"         , prefix_hash},
                {"account_id"          , account_id},
                {"total_received"      , total_received},
                {"total_sent"          , total_sent},
                {"height"              , height},
                {"payment_id"          , payment_id},
                {"coinbase"            , bool {coinbase}},
                {"mixin"               , mixin},
                {"timestamp"           , timestamp}
        };

        return j;
    }

    static DateTime
    timestamp_to_DateTime(time_t timestamp)
    {
        return DateTime(timestamp);
    }

    friend std::ostream& operator<< (std::ostream& stream, const XmrTransaction& acc);

};

ostream& operator<< (std::ostream& os, const XmrTransaction& acc) {
    os << "XmrTransactions: " << acc.to_json().dump() << '\n';
    return os;
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
    to_json() const
    {
        json j {{"id"                  , id},
                {"account_id"          , account_id},
                {"tx_id"               , tx_id},
                {"out_pub_key"         , out_pub_key},
                {"tx_pub_key"          , tx_pub_key},
                {"amount"              , amount},
                {"global_index"        , global_index},
                {"out_index"           , out_index},
                {"mixin"               , mixin},
                {"timestamp"           , timestamp}
        };

        return j;
    }


    friend std::ostream& operator<< (std::ostream& stream, const XmrOutput& out);

};

ostream& operator<< (std::ostream& os, const XmrOutput& out) {
    os << "XmrOutputs: " << out.to_json().dump() << '\n';
    return os;
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


    static constexpr const char* INSERT_STMT = R"(
      INSERT IGNORE INTO `Inputs` (`account_id`, `tx_id`, `output_id`,
                                `key_image`, `amount` , `timestamp`)
                        VALUES (%0q, %1q, %2q,
                                %3q, %4q, %5q);
    )";



    using Inputs::Inputs;

    json
    to_json() const
    {
        json j {{"id"                  , id},
                {"account_id"          , account_id},
                {"tx_id"               , tx_id},
                {"output_id"           , output_id},
                {"key_image"           , key_image},
                {"amount"              , amount},
                {"timestamp"           , timestamp}
        };

        return j;
    }


    friend std::ostream& operator<< (std::ostream& stream, const XmrInput& out);

};

ostream& operator<< (std::ostream& os, const XmrInput& out) {
    os << "XmrInput: " << out.to_json().dump() << '\n';
    return os;
};



// this is MySQL VIEW, based on the Transactions,
// Outputs and Inputs tables
sql_create_13(TransactionsWithOutsAndIns, 1, 2,
             sql_bigint_unsigned, tx_id,
             sql_bigint_unsigned, account_id,
             sql_varchar        , out_pub_key,
             sql_bigint_unsigned, amount,
             sql_bigint_unsigned, out_index,
             sql_bigint_unsigned, global_index,
             sql_varchar        , hash,
             sql_varchar        , prefix_hash,
             sql_varchar        , tx_pub_key,
             sql_timestamp      , timestamp,
             sql_bigint_unsigned, height,
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
    to_json() const
    {

        json j {{"tx_id"               , tx_id},
                {"account_id"          , account_id},
                {"amount"              , amount},
                {"tx_pub_key"          , tx_pub_key},
                {"out_pub_key"         , out_pub_key},
                {"global_index"        , global_index},
                {"tx_hash"             , hash},
                {"tx_prefix_hash"      , prefix_hash},
                {"out_index"           , out_index},
                {"timestamp"           , timestamp},
                {"height"              , height},
                {"spend_key_images"    , json::array()},
                {"key_image"           , key_image_to_string()},
                {"mixin"               , mixin}
        };

        return j;
    }

    json
    spent_output() const
    {
        json j {{"amount"    , amount},
                {"key_image" , key_image_to_string()},
                {"tx_pub_key", tx_pub_key},
                {"out_index" , out_index},
                {"mixin"     , mixin}
        };

        return j;
    }


    string key_image_to_string() const
    {
        string key_image_str {"NULL"};

        if (!key_image.is_null)
        {
            key_image_str = key_image.data;
        }

        return key_image_str;
    }


    friend std::ostream& operator<< (std::ostream& stream,
                                     const XmrTransactionWithOutsAndIns& out);

};

ostream& operator<< (std::ostream& os, const XmrTransactionWithOutsAndIns& out) {
    os << "XmrTransactionWithOutsAndIns: " << out.to_json().dump() << '\n';
    return os;
};


}


#endif //RESTBED_XMR_SSQLSES_H
