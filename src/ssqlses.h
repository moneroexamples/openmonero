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
    using Accounts::Accounts;

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


sql_create_10(Transactions, 1, 2,
              sql_bigint_unsigned, id,
              sql_varchar        , hash,
              sql_bigint_unsigned, account_id,
              sql_bigint_unsigned, total_received,
              sql_bigint_unsigned, total_sent,
              sql_bigint_unsigned, unlock_time,
              sql_bigint_unsigned, height,
              sql_bool           , coinbase,
              sql_bigint_unsigned, mixin,
              sql_timestamp      , timestamp);



struct XmrTransaction : public Transactions
{
    using Transactions::Transactions;

    // viewkey is not stored in mysql db or anywhere
    // so need to be populated when user logs in.
    string viewkey;

    json
    to_json() const
    {
        json j {{"id"                  , id},
                {"hash"                , hash},
                {"account_id"          , account_id},
                {"total_received"      , total_received},
                {"total_sent"          , total_sent},
                {"height"              , height},
                {"timestamp"           , timestamp}
        };

        return j;
    }

    friend std::ostream& operator<< (std::ostream& stream, const XmrTransaction& acc);

};

ostream& operator<< (std::ostream& os, const XmrTransaction& acc) {
    os << "XmrTransactions: " << acc.to_json().dump() << '\n';
    return os;
};



}


#endif //RESTBED_XMR_SSQLSES_H
