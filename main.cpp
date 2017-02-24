#include "src/CmdLineOptions.h"
#include "src/MicroCore.h"
#include "src/YourMoneroRequests.h"

#include <iostream>
#include <memory>
#include <cstdlib>

using namespace std;
using namespace restbed;

using boost::filesystem::path;

int
main(int ac, const char* av[])
{

// get command line options
xmreg::CmdLineOptions opts {ac, av};

auto help_opt         = opts.get_option<bool>("help");

// if help was chosen, display help text and finish
if (*help_opt)
{
    return EXIT_SUCCESS;
}

auto do_not_relay_opt = opts.get_option<bool>("do-not-relay");
auto testnet_opt      = opts.get_option<bool>("testnet");
auto port_opt         = opts.get_option<string>("port");
auto bc_path_opt      = opts.get_option<string>("bc-path");
auto config_file_opt  = opts.get_option<string>("config-file");


bool testnet          = *testnet_opt;
bool do_not_relay     = *do_not_relay_opt;

//cast port number in string to uint16
uint16_t app_port   = boost::lexical_cast<uint16_t>(*port_opt);

// get blockchain path
path blockchain_path;

if (!xmreg::get_blockchain_path(bc_path_opt, blockchain_path, testnet))
{
    cerr << "Error getting blockchain path." << endl;
    return EXIT_FAILURE;
}

cout << "Blockchain path: " << blockchain_path.string() << endl;

// check if config-file provided exist
if (!boost::filesystem::exists(*config_file_opt))
{
    cerr << "Config file " << *config_file_opt
         << " does not exist" << endl;
    return EXIT_FAILURE;
}

nlohmann::json config_json;

try
{
    // try reading and parsing confing file provided
    std::ifstream i(*config_file_opt);
    i >> config_json;
}
catch (const std::exception& e)
{
    cerr << "Error reading confing file "
         << *config_file_opt << ": "
         << e.what() << endl;
    return EXIT_FAILURE;
}

string deamon_url = testnet
                    ? config_json["daemon-url"]["testnet"]
                    : config_json["daemon-url"]["mainnet"];

// set mysql/mariadb connection details
xmreg::MySqlConnector::url      = config_json["database"]["url"];
xmreg::MySqlConnector::username = config_json["database"]["user"];
xmreg::MySqlConnector::password = config_json["database"]["password"];
xmreg::MySqlConnector::dbname   = config_json["database"]["dbname"];

// set blockchain status monitoring thread parameters
xmreg::CurrentBlockchainStatus::blockchain_path
        =  blockchain_path.string();
xmreg::CurrentBlockchainStatus::testnet
        = testnet;
xmreg::CurrentBlockchainStatus::do_not_relay
        = do_not_relay;
xmreg::CurrentBlockchainStatus::deamon_url
        = deamon_url;
xmreg::CurrentBlockchainStatus::refresh_block_status_every_seconds
        = config_json["refresh_block_status_every_seconds"];
xmreg::CurrentBlockchainStatus::search_thread_life_in_seconds
        = config_json["search_thread_life_in_seconds"];
xmreg::CurrentBlockchainStatus::import_fee
        = config_json["wallet_import"]["fee"];

if (testnet)
{
    xmreg::CurrentBlockchainStatus::import_payment_address
            = config_json["wallet_import"]["testnet"]["address"];
    xmreg::CurrentBlockchainStatus::import_payment_viewkey
            = config_json["wallet_import"]["testnet"]["viewkey"];
}
else
{
    xmreg::CurrentBlockchainStatus::import_payment_address
            = config_json["wallet_import"]["mainnet"]["address"];
    xmreg::CurrentBlockchainStatus::import_payment_viewkey
            = config_json["wallet_import"]["mainnet"]["viewkey"];
}


// since CurrentBlockchainStatus class monitors current status
// of the blockchain (e.g., current height), its seems logical to
// make static objects for accessing the blockchain in this class.
// this way monero accessing blockchain variables (i.e. mcore and core_storage)
// are not passed around like crazy everywhere.
// There are here, and this is the only class that
// has direct access to blockchain and talks (using rpc calls)
// with the deamon.
if (!xmreg::CurrentBlockchainStatus::init_monero_blockchain())
{
    cerr << "Error accessing blockchain." << endl;
    return EXIT_FAILURE;
}

// launch the status monitoring thread so that it keeps track of blockchain
// info, e.g., current height. Information from this thread is used
// by tx searching threads that are launched for each user independently,
// when they log back or create new account.
xmreg::CurrentBlockchainStatus::start_monitor_blockchain_thread();




// create REST JSON API services

// Open Monero frontend url.  Frontend url must match this value in
// server. Otherwise CORS problems between frontend and backend can occure
xmreg::YourMoneroRequests::frontend_url
        = config_json["frontend-url"];

xmreg::YourMoneroRequests your_xmr(
        shared_ptr<xmreg::MySqlAccounts>(new xmreg::MySqlAccounts{}));

auto login                 = your_xmr.make_resource(
        &xmreg::YourMoneroRequests::login,
        "/login");

auto get_address_txs       = your_xmr.make_resource(
        &xmreg::YourMoneroRequests::get_address_txs,
        "/get_address_txs");

auto get_address_info      = your_xmr.make_resource(
        &xmreg::YourMoneroRequests::get_address_info,
        "/get_address_info");

auto get_unspent_outs      = your_xmr.make_resource(
        &xmreg::YourMoneroRequests::get_unspent_outs,
        "/get_unspent_outs");

auto get_random_outs       = your_xmr.make_resource(
        &xmreg::YourMoneroRequests::get_random_outs,
        "/get_random_outs");

auto submit_raw_tx         = your_xmr.make_resource(
        &xmreg::YourMoneroRequests::submit_raw_tx,
        "/submit_raw_tx");

auto import_wallet_request = your_xmr.make_resource(
        &xmreg::YourMoneroRequests::import_wallet_request,
        "/import_wallet_request");

auto get_version           = your_xmr.make_resource(
        &xmreg::YourMoneroRequests::get_version,
        "/get_version");

// restbed service
Service service;

// Open Monero API we publish to the frontend
service.publish(login);
service.publish(get_address_txs);
service.publish(get_address_info);
service.publish(get_unspent_outs);
service.publish(get_random_outs);
service.publish(submit_raw_tx);
service.publish(import_wallet_request);
service.publish(get_version);

auto settings = make_shared<Settings>( );

if (config_json["ssl"]["enable"])
{
    auto ssl_settings = make_shared<SSLSettings>( );

    ssl_settings->set_http_disabled( true );
    ssl_settings->set_port(app_port);
    ssl_settings->set_private_key( Uri( "file:///tmp/mwo.key" ) );
    ssl_settings->set_certificate( Uri( "file:///tmp/mwo.crt" ) );
    ssl_settings->set_temporary_diffie_hellman( Uri( "file:///tmp/dh2048.pem" ) );

    settings->set_ssl_settings(ssl_settings);

    cout << "Start the service at https://localhost:" << app_port << endl;
}
else
{
    settings->set_port(app_port);

    cout << "Start the service at http://localhost:" << app_port  << endl;
}


service.start(settings);

return EXIT_SUCCESS;
}