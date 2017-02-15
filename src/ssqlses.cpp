//
// Created by mwo on 7/01/17.
//

#include "ssqlses.h"


namespace xmreg
{




json
XmrAccount::to_json() const
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

ostream& operator<< (std::ostream& os, const XmrAccount& acc)
{
    os << "XmrAccount: " << acc.to_json().dump() << '\n';
    return os;
};


json
XmrTransaction::to_json() const
{
    json j {{"id"                  , id},
            {"hash"                , hash},
            {"prefix_hash"         , prefix_hash},
            {"account_id"          , account_id},
            {"total_received"      , total_received},
            {"total_sent"          , total_sent},
            {"height"              , height},
            {"payment_id"          , payment_id},
            {"unlock_time"         , unlock_time},
            {"coinbase"            , bool {coinbase}},
            {"spendable"           , bool {spendable}},
            {"mixin"               , mixin},
            {"timestamp"           , timestamp}
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
            {"timestamp"           , timestamp}
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
            {"timestamp"           , timestamp}
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