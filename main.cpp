#include "src/CmdLineOptions.h"
#include "src/MicroCore.h"
#include "src/MySqlAccounts.h"
#include "src/YourMoneroRequests.h"
#include "src/tools.h"
#include "src/TxSearch.h"

#include "ext/restbed/source/restbed"


#include <iostream>
#include <memory>
#include <cstdlib>

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
auto use_ssl_opt       = opts.get_option<bool>("use-ssl");

// if help was chosen, display help text and finish
if (*help_opt)
{
    return EXIT_SUCCESS;
}

bool testnet       {*testnet_opt};
bool use_ssl       {*use_ssl_opt};

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


xmreg::MySqlConnector::url       = "127.0.0.1";
xmreg::MySqlConnector::username  = "root";
xmreg::MySqlConnector::password  = "root";
xmreg::MySqlConnector::dbname    = "openmonero";

// setup blockchain status monitoring thread
xmreg::CurrentBlockchainStatus::set_blockchain_path(blockchain_path.string());
xmreg::CurrentBlockchainStatus::set_testnet(testnet);
xmreg::CurrentBlockchainStatus::refresh_block_status_every_seconds = 30;

// since CurrentBlockchainStatus class monitors current status
// of the blockchain (e.g, current height), its seems logical to
// make static objects for accessing the blockchain in this class.
// this way monero accessing blockchain variables (i.e. mcore and core_storage)
// are not passed around like crazy everywhere.
// There are here, and this is the only class that
// has direct access to blockchain.
if (!xmreg::CurrentBlockchainStatus::init_monero_blockchain())
{
    cerr << "Error accessing blockchain." << endl;
    return EXIT_FAILURE;
}

// launch the status monitoring thread so that it keeps track of blockchain
// info, e.g., current height. Information from this thread is used
// by tx searching threads that are launch for each user independent,
// when they login back or create new account.
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

auto get_unspent_outs      = your_xmr.make_resource(
        &xmreg::YourMoneroRequests::get_unspent_outs     , "/get_unspent_outs");

auto get_random_outs      = your_xmr.make_resource(
        &xmreg::YourMoneroRequests::get_random_outs     , "/get_random_outs");

auto submit_raw_tx      = your_xmr.make_resource(
        &xmreg::YourMoneroRequests::submit_raw_tx     , "/submit_raw_tx");

auto import_wallet_request = your_xmr.make_resource(
        &xmreg::YourMoneroRequests::import_wallet_request, "/import_wallet_request");

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
service.publish(get_unspent_outs);
service.publish(get_random_outs);
service.publish(submit_raw_tx);
service.publish(import_wallet_request);

service.start(settings);

return EXIT_SUCCESS;
}