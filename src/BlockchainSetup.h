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
