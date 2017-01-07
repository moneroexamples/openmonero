//
// Created by mwo on 7/01/17.
//

#ifndef RESTBED_XMR_SSQLS_RAW_H
#define RESTBED_XMR_SSQLS_RAW_H

#include <mysql++/mysql++.h>
#include <mysql++/ssqls.h>


namespace xmreg
{

using namespace mysqlpp;

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



sql_create_7(Inputs, 1, 4,
             sql_bigint_unsigned, id,
             sql_bigint_unsigned, account_id,
             sql_bigint_unsigned, tx_id,
             sql_bigint_unsigned, output_id,
             sql_varchar        , key_image,
             sql_bigint_unsigned, amount,
             sql_timestamp      , timestamp);



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


// this is MySQL VIEW, based on the Transactions,
// Outputs and Inputs tables
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


};

#endif //RESTBED_XMR_SSQLSES_H