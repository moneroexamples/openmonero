//
// Created by mwo on 10/07/18.
//

#include "BlockchainSetup.h"

namespace xmreg
{

bool
BlockchainSetup::parse_addr_and_viewkey()
{
    if (import_payment_address_str.empty() || import_payment_viewkey_str.empty())
        return false;

    if (!parse_str_address(
            import_payment_address_str,
            import_payment_address,
            net_type))
    {
        cerr << "Cant parse address_str: "
             << import_payment_address_str
             << endl;
        return false;
    }

    if (!xmreg::parse_str_secret_key(
            import_payment_viewkey_str,
            import_payment_viewkey))
    {
        cerr << "Cant parse the viewkey_str: "
             << import_payment_viewkey_str
             << endl;
        return false;
    }

    return true;
}

}