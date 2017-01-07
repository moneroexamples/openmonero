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

ostream& operator<< (std::ostream& os, const XmrAccount& acc) {
    os << "XmrAccount: " << acc.to_json().dump() << '\n';
    return os;
};


}