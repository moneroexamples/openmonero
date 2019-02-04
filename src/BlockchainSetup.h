//
// Created by mwo on 10/07/18.
//

#ifndef OPENMONERO_BLOCKCHAINSETUP_H
#define OPENMONERO_BLOCKCHAINSETUP_H

#include "src/monero_headers.h"
#include "utils.h"

#include <string>

namespace xmreg
{

using namespace crypto;
using namespace cryptonote;
using namespace std;

using chrono::seconds;

class BlockchainSetup
{
public:

    BlockchainSetup() = default;

    BlockchainSetup(network_type _net_type,
                    bool _do_not_relay,
                    string _config_path);

    BlockchainSetup(network_type _net_type,
                    bool _do_not_relay,
                    nlohmann::json _config);

    string blockchain_path;

    string deamon_url;

    network_type net_type;

    bool do_not_relay {false};

    seconds refresh_block_status_every {10};
    seconds search_thread_life {120};
    seconds mysql_ping_every {300};

    uint64_t blocks_search_lookahead {200};

    uint64_t max_number_of_blocks_to_import {132000};

    uint64_t blockchain_treadpool_size {0};

    string   import_payment_address_str;
    string   import_payment_viewkey_str;

    uint64_t import_fee {0};

    uint64_t spendable_age {10};
    uint64_t spendable_age_coinbase {60};

    address_parse_info import_payment_address;
    secret_key         import_payment_viewkey;

    void
    parse_addr_and_viewkey();

    void
    get_blockchain_path();

    json
    get_config() const {return config_json;}

    static string
    get_network_name(network_type n_type);

    static json
    read_config(string config_json_path);

private:

    void _init();

    string config_path;
    nlohmann::json config_json;
};

}


#endif //OPENMONERO_BLOCKCHAINSETUP_H
