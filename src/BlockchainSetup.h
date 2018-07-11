//
// Created by mwo on 10/07/18.
//

#ifndef OPENMONERO_BLOCKCHAINSETUP_H
#define OPENMONERO_BLOCKCHAINSETUP_H

#include "monero_headers.h"
#include "tools.h"

#include <string>

namespace xmreg
{

using namespace crypto;
using namespace cryptonote;
using namespace std;

class BlockchainSetup
{
public:

    string blockchain_path;

    string deamon_url;

    network_type net_type;

    bool do_not_relay;

    uint64_t refresh_block_status_every_seconds;

    uint64_t blocks_search_lookahead;

    uint64_t max_number_of_blocks_to_import;

    uint64_t search_thread_life_in_seconds;

    string   import_payment_address_str;
    string   import_payment_viewkey_str;

    uint64_t import_fee;

    uint64_t spendable_age;
    uint64_t spendable_age_coinbase;

    address_parse_info import_payment_address;
    secret_key         import_payment_viewkey;

    bool
    parse_addr_and_viewkey();
};

}


#endif //OPENMONERO_BLOCKCHAINSETUP_H
