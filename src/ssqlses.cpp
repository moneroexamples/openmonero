//
// Created by mwo on 7/01/17.
//

#include "ssqlses.h"


namespace xmreg
{


ostream& operator<< (std::ostream& os, const Table& data)
{
    os << data.table_name() << ": " << data.to_json().dump() << '\n';
    return os;
};

json
XmrAccount::to_json() const
{
    json j {{"id"                     , id},
            {"address"                , address},
            {"viewkey"                , viewkey},
            {"scanned_block_height"   , scanned_block_height},
            {"scanned_block_timestamp", static_cast<uint64_t>(scanned_block_timestamp)},
            {"start_height"           , start_height}
    };

    return j;
}

json
XmrTransaction::to_json() const
{
    json j {{"id"                  , id},
            {"hash"                , hash},
            {"prefix_hash"         , prefix_hash},
            {"tx_pub_key"          , tx_pub_key},
            {"account_id"          , account_id},
            {"total_received"      , total_received},
            {"total_sent"          , total_sent},
            {"height"              , height},
            {"payment_id"          , payment_id},
            {"unlock_time"         , unlock_time},
            {"coinbase"            , bool {coinbase}},
            {"is_rct"              , bool {is_rct}},
            {"rct_type"            , rct_type},
            {"spendable"           , bool {spendable}},
            {"mixin"               , mixin},
            {"timestamp"           , static_cast<uint64_t>(timestamp)}
    };

    return j;
}

DateTime
XmrTransaction::timestamp_to_DateTime(time_t timestamp)
{
    return DateTime(timestamp);
}

ostream& operator<< (std::ostream& os, const XmrTransaction& acc)
{
    os << "XmrTransactions: " << acc.to_json().dump() << '\n';
    return os;
};


json
XmrOutput::to_json() const
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
            {"timestamp"           , static_cast<uint64_t>(timestamp)}
    };

    return j;
}


ostream& operator<< (std::ostream& os, const XmrOutput& out) {
    os << "XmrOutputs: " << out.to_json().dump() << '\n';
    return os;
};


json
XmrInput::to_json() const
{
    json j {{"id"                  , id},
            {"account_id"          , account_id},
            {"tx_id"               , tx_id},
            {"output_id"           , output_id},
            {"key_image"           , key_image},
            {"amount"              , amount},
            {"timestamp"           , static_cast<uint64_t>(timestamp)}
    };

    return j;
}



ostream& operator<< (std::ostream& os, const XmrInput& out)
{
    os << "XmrInput: " << out.to_json().dump() << '\n';
    return os;
};

json
XmrPayment::to_json() const
{
    json j {{"id"               , id},
            {"address"          , address},
            {"payment_id"       , payment_id},
            {"tx_hash"          , tx_hash},
            {"request_fulfilled", bool {request_fulfilled}},
            {"payment_address"  , payment_address},
            {"import_fee"       , import_fee}
    };

    return j;
}


ostream& operator<< (std::ostream& os, const XmrPayment& out) {
    os << "XmrPayment: " << out.to_json().dump() << '\n';
    return os;
};



}