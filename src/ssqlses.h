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

/**
 * Base class for all mysql table based classes that we use.
 */
class Table
{
public:

    virtual string table_name() const = 0;
    virtual json to_json()      const = 0;

    friend std::ostream& operator<< (std::ostream& stream, const Table& data);
};

sql_create_8(Accounts, 1, 2,
             sql_bigint_unsigned, id,
             sql_varchar        , address,
             sql_char           , viewkey_hash,
             sql_bigint_unsigned, scanned_block_height,
             sql_timestamp      , scanned_block_timestamp,
             sql_bigint_unsigned, start_height,
             sql_timestamp      , created,
             sql_timestamp      , modified);


struct XmrAccount : public Accounts, Table
{

    static constexpr const char* SELECT_STMT = R"(
        SELECT * FROM `Accounts` WHERE `address` = (%0q)
    )";

    static constexpr const char* SELECT_STMT2 = R"(
        SELECT * FROM `Accounts` WHERE `id` = (%0q)
    )";

    static constexpr const char* INSERT_STMT = R"(
        INSERT INTO `Accounts` (`address`, `viewkey_hash`,
                                `scanned_block_height`,
                                `scanned_block_timestamp`, `start_height`)
                                VALUES
                                (%0q, %1q, %2q, %3q, %4q);
    )";

    using Accounts::Accounts;

    // viewkey is not stored in mysql db or anywhere
    // so need to be populated when user logs in.
    string viewkey;

    string table_name() const override { return this->table();};

    json to_json() const override;

};

sql_create_17(Transactions, 1, 2,
              sql_bigint_unsigned, id,
              sql_varchar        , hash,
              sql_varchar        , prefix_hash,
              sql_varchar        , tx_pub_key,
              sql_bigint_unsigned, account_id,
              sql_bigint_unsigned, blockchain_tx_id,
              sql_bigint_unsigned, total_received,
              sql_bigint_unsigned, total_sent,
              sql_bigint_unsigned, unlock_time,
              sql_bigint_unsigned, height,
              sql_bool           , coinbase,
              sql_bool           , is_rct,
              sql_int            , rct_type,
              sql_bool           , spendable,
              sql_varchar        , payment_id,
              sql_bigint_unsigned, mixin,
              sql_timestamp      , timestamp);


struct XmrTransaction : public Transactions, Table
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

    static constexpr const char* DELETE_STMT = R"(
       DELETE FROM `Transactions` WHERE `id` = (%0q)
    )";

    static constexpr const char* INSERT_STMT = R"(
        INSERT IGNORE INTO `Transactions` (`hash`, `prefix_hash`, `tx_pub_key`, `account_id`, 
                                           `blockchain_tx_id`,
                                           `total_received`, `total_sent`, `unlock_time`,
                                           `height`, `coinbase`, `is_rct`, `rct_type`,
                                           `spendable`,
                                           `payment_id`, `mixin`, `timestamp`)
                                VALUES (%0q, %1q, %2q, %3q,
                                        %4q, 
                                        %5q, %6q, %7q, 
                                        %8q, %9q, %10q, %11q,
                                        %12q, 
                                        %13q, %14q, %15q);
    )";

    static constexpr const char* MARK_AS_SPENDABLE_STMT = R"(
       UPDATE `Transactions` SET `spendable` = 1,  `timestamp` = CURRENT_TIMESTAMP
                             WHERE `id` = %0q;
    )";

    static constexpr const char* SUM_XMR_RECIEVED = R"(
        SELECT SUM(`total_received`) AS total_received
               FROM `Transactions`
               WHERE `account_id` = %0q
               GROUP BY `account_id`
    )";




    using Transactions::Transactions;


    static DateTime
    timestamp_to_DateTime(time_t timestamp);

    string table_name() const override { return this->table();};

    json to_json() const override;

};

sql_create_13(Outputs, 1, 3,
              sql_bigint_unsigned, id,
              sql_bigint_unsigned, account_id,
              sql_bigint_unsigned, tx_id,
              sql_varchar        , out_pub_key,
              sql_varchar        , rct_outpk,
              sql_varchar        , rct_mask,
              sql_varchar        , rct_amount,
              sql_varchar        , tx_pub_key,
              sql_bigint_unsigned, amount,
              sql_bigint_unsigned, global_index,
              sql_bigint_unsigned, out_index,
              sql_bigint_unsigned, mixin,
              sql_timestamp      , timestamp);

struct XmrOutput : public Outputs, Table
{

    static constexpr const char* SELECT_STMT = R"(
      SELECT * FROM `Outputs` WHERE `account_id` = (%0q)
    )";

    static constexpr const char* SELECT_STMT2 = R"(
      SELECT * FROM `Outputs` WHERE `tx_id` = (%0q) ORDER BY amount
    )";

    static constexpr const char* SELECT_STMT3 = R"(
      SELECT * FROM `Outputs` WHERE `id` = (%0q)
    )";

    static constexpr const char* EXIST_STMT = R"(
      SELECT * FROM `Outputs` WHERE `out_pub_key` = (%0q)
    )";

    static constexpr const char* INSERT_STMT = R"(
      INSERT IGNORE INTO `Outputs` (`account_id`, `tx_id`, `out_pub_key`,
                                     `tx_pub_key`,
                                     `rct_outpk`, `rct_mask`, `rct_amount`,
                                     `amount`, `global_index`,
                                     `out_index`, `mixin`, `timestamp`)
                            VALUES (%0q, %1q, %2q,
                                    %3q,
                                    %4q, %5q, %6q,
                                    %7q, %8q,
                                    %9q, %10q, %11q);
    )";



    using Outputs::Outputs;

    string
    get_rct() const
    {
        return rct_outpk + rct_mask + rct_amount;
    }


    string table_name() const override { return this->table();};

    json to_json() const override;

};


sql_create_7(Inputs, 1, 4,
             sql_bigint_unsigned, id,
             sql_bigint_unsigned, account_id,
             sql_bigint_unsigned, tx_id,
             sql_bigint_unsigned, output_id,
             sql_varchar        , key_image,
             sql_bigint_unsigned, amount,
             sql_timestamp      , timestamp);


struct XmrInput : public Inputs, Table
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

    string table_name() const override { return this->table();};

    json to_json() const override;

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


struct XmrPayment : public Payments, Table
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

    string table_name() const override { return this->table();};

    json to_json() const override;

};


}


#endif //RESTBED_XMR_SSQLSES_H
