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
sql_create_11(Accounts, 1, 2,
              sql_bigint_unsigned, id,
              sql_varchar        , address,
              sql_bigint_unsigned, total_received,
              sql_bigint_unsigned, scanned_height,
              sql_bigint_unsigned, scanned_block_height,
              sql_bigint_unsigned, start_height,
              sql_bigint_unsigned, transaction_height,
              sql_bigint_unsigned, blockchain_height,
              sql_bigint_unsigned, total_sent,
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
        INSERT INTO `Accounts` (`address`, `scanned_block_height`) VALUES (%0q, %1q);
    )";


    using Accounts::Accounts;

    XmrAccount():Accounts()
    {
        address = "";
        scanned_height = 0;
        scanned_block_height = 0;
        start_height = 0;
        transaction_height = 0;
        blockchain_height = 0;
        total_sent = 0;
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
                {"total_sent"          , total_sent},
                {"scanned_block_height", scanned_block_height},
                {"blockchain_height"   , blockchain_height}
        };

        return j;
    }


    friend std::ostream& operator<< (std::ostream& stream, const XmrAccount& acc);

};

ostream& operator<< (std::ostream& os, const XmrAccount& acc) {
    os << "XmrAccount: " << acc.to_json().dump() << '\n';
    return os;
};


sql_create_11(Transactions, 1, 2,
              sql_bigint_unsigned, id,
              sql_varchar        , hash,
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

    static constexpr const char* INSERT_STMT = R"(
        INSERT IGNORE INTO `Transactions` (`hash`, `account_id`, `total_received`,
                                    `total_sent`, `unlock_time`, `height`,
                                    `coinbase`, `payment_id`, `mixin`,
                                    `timestamp`)
                                VALUES (%0q, %1q, %2q,
                                        %3q, %4q, %5q,
                                        %6q, %7q, %8q,
                                        %9q);
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
                {"account_id"          , account_id},
                {"total_received"      , total_received},
                {"total_sent"          , total_sent},
                {"height"              , height},
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


sql_create_8(Outputs, 1, 3,
             sql_bigint_unsigned, id,
             sql_bigint_unsigned, account_id,
             sql_bigint_unsigned, tx_id,
             sql_varchar        , out_pub_key,
             sql_varchar        , tx_pub_key,
             sql_bigint_unsigned, out_index,
             sql_bigint_unsigned, mixin,
             sql_timestamp      , timestamp);


struct XmrOutput : public Outputs
{

    static constexpr const char* SELECT_STMT = R"(
      SELECT * FROM `Outputs` WHERE `account_id` = (%0q)
    )";

    static constexpr const char* EXIST_STMT = R"(
      SELECT 1 FROM `Outputs` WHERE `out_pub_key` == (%0q)
    )";

    static constexpr const char* INSERT_STMT = R"(
      INSERT IGNORE INTO `Outputs` (`account_id`, `tx_id`, `out_pub_key`, `tx_pub_key`,
                                    `out_index`, `mixin`, `timestamp`)
                            VALUES (%0q, %1q, %2q, %3q,
                                    %4q, %5q, %6q);
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


sql_create_6(Inputs, 1, 4,
             sql_bigint_unsigned, id,
             sql_bigint_unsigned, account_id,
             sql_bigint_unsigned, tx_id,
             sql_bigint_unsigned, output_id,
             sql_varchar        , key_image,
             sql_timestamp      , timestamp);


struct XmrInput : public Inputs
{

    static constexpr const char* SELECT_STMT = R"(
     SELECT * FROM `Inputs` WHERE `account_id` = (%0q)
    )";

    static constexpr const char* EXIST_STMT = R"(
     SELECT 1 FROM `Inputs` WHERE `key_image` == (%0q)
    )";

    static constexpr const char* INSERT_STMT = R"(
      INSERT IGNORE INTO `Inputs` (`account_id`, `tx_id`, `output_id`,
                                `key_image`, `timestamp`)
                        VALUES (%0q, %1q, %2q,
                                %3q, %4q);
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



}


#endif //RESTBED_XMR_SSQLSES_H
