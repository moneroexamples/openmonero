#include <iostream>

#include <memory>
#include <cstdlib>

#include "ext/restbed/source/restbed"


#include "src/CmdLineOptions.h"
#include "src/MicroCore.h"
#include "src/MySqlConnector.h"
#include "src/YourMoneroRequests.h"
#include "src/tools.h"
#include "src/TxSearch.h"

using namespace std;
using namespace restbed;

using boost::filesystem::path;

// needed for log system of momero
namespace epee {
    unsigned int g_test_dbg_lock_sleep = 0;
}


int
main(int ac, const char* av[])
{

// get command line options
xmreg::CmdLineOptions opts {ac, av};

auto help_opt          = opts.get_option<bool>("help");
auto testnet_opt       = opts.get_option<bool>("testnet");

// if help was chosen, display help text and finish
if (*help_opt)
{
    return EXIT_SUCCESS;
}

bool testnet       {*testnet_opt};

auto port_opt           = opts.get_option<string>("port");
auto bc_path_opt        = opts.get_option<string>("bc-path");

//cast port number in string to uint16
uint16_t app_port = boost::lexical_cast<uint16_t>(*port_opt);

// get blockchain path
path blockchain_path;

if (!xmreg::get_blockchain_path(bc_path_opt, blockchain_path, testnet))
{
    cerr << "Error getting blockchain path." << endl;
    return EXIT_FAILURE;
}

cout << "Blockchain path: " << blockchain_path.string() << endl;

// enable basic monero log output
xmreg::enable_monero_log();


// create instance of our MicroCore
// and make pointer to the Blockchain
xmreg::MicroCore mcore;
cryptonote::Blockchain* core_storage;

// initialize mcore and core_storage
if (!xmreg::init_blockchain(blockchain_path.string(),
                            mcore, core_storage))
{
    cerr << "Error accessing blockchain." << endl;
    return EXIT_FAILURE;
}

// setup blockchain status monitoring thread
xmreg::CurrentBlockchainStatus::set_blockchain_path(blockchain_path.string());
xmreg::CurrentBlockchainStatus::set_testnet(false);
xmreg::CurrentBlockchainStatus::refresh_block_status_every_seconds = 30;

// launch the status monitoring thread so that it keeps track of blockchain
// info, e.g., current height. Information from this thread is used
// by tx searching threads that are launch upon for each user independent.
xmreg::CurrentBlockchainStatus::start_monitor_blockchain_thread();

xmreg::YourMoneroRequests::show_logs = true;


xmreg::YourMoneroRequests your_xmr(
        shared_ptr<xmreg::MySqlAccounts>(new xmreg::MySqlAccounts{}));



auto login                 = your_xmr.make_resource(
        &xmreg::YourMoneroRequests::login                , "/login");

auto get_address_txs       = your_xmr.make_resource(
        &xmreg::YourMoneroRequests::get_address_txs      , "/get_address_txs");

auto get_address_info      = your_xmr.make_resource(
        &xmreg::YourMoneroRequests::get_address_info     , "/get_address_info");

auto import_wallet_request = your_xmr.make_resource(
        &xmreg::YourMoneroRequests::import_wallet_request, "/import_wallet_request");

bool use_ssl {false};

auto settings = make_shared< Settings >( );

if (use_ssl)
{
    auto ssl_settings = make_shared< SSLSettings >( );

    ssl_settings->set_http_disabled( true );
    ssl_settings->set_port(1984);
    ssl_settings->set_private_key( Uri( "file:///tmp/mwo.key" ) );
    ssl_settings->set_certificate( Uri( "file:///tmp/mwo.crt" ) );
    ssl_settings->set_temporary_diffie_hellman( Uri( "file:///tmp/dh2048.pem" ) );

    settings->set_ssl_settings(ssl_settings);
}
else
{
    settings->set_port(1984);
}

cout << "Start the service at http://localhost:1984" << endl;

Service service;

service.publish(login);
service.publish(get_address_txs);
service.publish(get_address_info);
service.publish(import_wallet_request);

service.start(settings);

return EXIT_SUCCESS;
}