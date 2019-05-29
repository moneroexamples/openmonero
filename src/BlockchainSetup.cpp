//
// Created by mwo on 10/07/18.
//

#include "BlockchainSetup.h"
#include "xmregcore/src/tools.h"

namespace xmreg
{

BlockchainSetup::BlockchainSetup(
        network_type _net_type,
        bool _do_not_relay,
        string _config_path)
        : net_type {_net_type},
          do_not_relay {_do_not_relay},
          config_path {_config_path}
{
    config_json = read_config(config_path);

    _init();
}


BlockchainSetup::BlockchainSetup(
        network_type _net_type,
        bool _do_not_relay,
        nlohmann::json _config)
    : net_type {_net_type},
      do_not_relay {_do_not_relay},
      config_json (_config)
{
    _init();
}

void
BlockchainSetup::parse_addr_and_viewkey()
{
    if (import_payment_address_str.empty()
            || import_payment_viewkey_str.empty())
        std::runtime_error("config address or viewkey are empty.");


    try
    {
        if (!parse_str_address(
                import_payment_address_str,
                import_payment_address,
                net_type))
        {
            throw std::runtime_error("Cant parse config address: "
                                     + import_payment_address_str);
        }


        if (!xmreg::parse_str_secret_key(
                import_payment_viewkey_str,
                import_payment_viewkey))
        {
            throw std::runtime_error("Cant parse config viewkey: "
                                     + import_payment_viewkey_str);
        }

    }
    catch (std::exception const& e)
    {
        throw;
    }

}

void
BlockchainSetup::get_blockchain_path()
{
    switch (net_type)
    {
        case network_type::MAINNET:
            blockchain_path = config_json["blockchain-path"]["mainnet"]
                    .get<string>();
            deamon_url = config_json["daemon-url"]["mainnet"];
            import_payment_address_str
                    = config_json["wallet_import"]["mainnet"]["address"];
            import_payment_viewkey_str
                    = config_json["wallet_import"]["mainnet"]["viewkey"];
            break;
        case network_type::TESTNET:
            blockchain_path = config_json["blockchain-path"]["testnet"]
                    .get<string>();
            deamon_url = config_json["daemon-url"]["testnet"];
            import_payment_address_str
                    = config_json["wallet_import"]["testnet"]["address"];
            import_payment_viewkey_str
                    = config_json["wallet_import"]["testnet"]["viewkey"];
            break;
        case network_type::STAGENET:
            blockchain_path = config_json["blockchain-path"]["stagenet"]
                    .get<string>();
            deamon_url = config_json["daemon-url"]["stagenet"];
            import_payment_address_str
                    = config_json["wallet_import"]["stagenet"]["address"];
            import_payment_viewkey_str
                    = config_json["wallet_import"]["stagenet"]["viewkey"];
            break;
        default:
            throw std::runtime_error("Invalid netowork type provided "
                               + std::to_string(static_cast<int>(net_type)));
    }

    if (!xmreg::get_blockchain_path(blockchain_path, net_type))
        throw std::runtime_error("Error getting blockchain path.");
}



string
BlockchainSetup::get_network_name(network_type n_type)
{
    switch (n_type)
    {
        case network_type::MAINNET:
            return "mainnet";
        case network_type::TESTNET:
            return "testnet";
        case network_type::STAGENET:
            return "stagenet";
        default:
            throw std::runtime_error("wrong network");
    }
}


json
BlockchainSetup::read_config(string config_json_path)
{
    // check if config-file provided exist
    if (!boost::filesystem::exists(config_json_path))
        throw std::runtime_error("Config file " + config_json_path
                                 + " does not exist");

    json _config;

    try
    {
        // try reading and parsing json config file provided
        std::ifstream i(config_json_path);
        i >> _config;
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(string{"Cant parse json string as json: "}
                                 + e.what());
    }

    return _config;
}



    void
BlockchainSetup::_init()
{
    refresh_block_status_every
            = seconds {config_json["refresh_block_status_every_seconds"]};
    blocks_search_lookahead
            = config_json["blocks_search_lookahead"];
    max_number_of_blocks_to_import
            = config_json["max_number_of_blocks_to_import"];
    search_thread_life
            = seconds {config_json["search_thread_life_in_seconds"]};
    import_fee
            = config_json["wallet_import"]["fee"];
    mysql_ping_every
            = seconds {config_json["mysql_ping_every_seconds"]};
    blockchain_treadpool_size 
            = config_json["blockchain_treadpool_size"];

    get_blockchain_path();

    parse_addr_and_viewkey();
}

}
