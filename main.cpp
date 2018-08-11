#include "src/om_log.h"
#include "src/CmdLineOptions.h"
#include "src/MicroCore.h"
#include "src/YourMoneroRequests.h"
#include "src/ThreadRAII.h"
#include "src/MysqlPing.h"

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

auto help_opt = opts.get_option<bool>("help");

// if help was chosen, display help text and finish
if (*help_opt)
{
    return EXIT_SUCCESS;
}

// setup monero logger
mlog_configure(mlog_get_default_log_path(""), true);
mlog_set_log("1");

string log_file  = *(opts.get_option<string>("log-file"));

// setup a logger for Open Monero

el::Configurations defaultConf;

defaultConf.setToDefault();

if (!log_file.empty())
{
    // setup openmonero log file
    defaultConf.setGlobally(el::ConfigurationType::Filename, log_file);
    defaultConf.setGlobally(el::ConfigurationType::ToFile, "true");
}

defaultConf.setGlobally(el::ConfigurationType::ToStandardOutput, "true");

el::Loggers::reconfigureLogger("openmonero", defaultConf);

OMINFO << "OpenMonero is starting";

auto do_not_relay_opt = opts.get_option<bool>("do-not-relay");
auto testnet_opt      = opts.get_option<bool>("testnet");
auto stagenet_opt     = opts.get_option<bool>("stagenet");
auto port_opt         = opts.get_option<string>("port");
auto config_file_opt  = opts.get_option<string>("config-file");


bool testnet          = *testnet_opt;
bool stagenet         = *stagenet_opt;
bool do_not_relay     = *do_not_relay_opt;

if (testnet && stagenet)
{   
    OMERROR << "testnet and stagenet cannot be specified at the same time!";
    return EXIT_FAILURE;
}

// get the network type
cryptonote::network_type nettype = testnet ?
  cryptonote::network_type::TESTNET : stagenet ?
  cryptonote::network_type::STAGENET : cryptonote::network_type::MAINNET;

// create blockchainsetup instance and set its parameters
// such as blockchain status monitoring thread parameters

xmreg::BlockchainSetup bc_setup {nettype, do_not_relay, *config_file_opt};

OMINFO << "Using blockchain path: " << bc_setup.blockchain_path;

nlohmann::json config_json = bc_setup.get_config();


//cast port number in string to uint16
uint16_t app_port   = boost::lexical_cast<uint16_t>(*port_opt);

// set mysql/mariadb connection details
xmreg::MySqlConnector::url      = config_json["database"]["url"];
xmreg::MySqlConnector::port     = config_json["database"]["port"];
xmreg::MySqlConnector::username = config_json["database"]["user"];
xmreg::MySqlConnector::password = config_json["database"]["password"];
xmreg::MySqlConnector::dbname   = config_json["database"]["dbname"];


// once we have all the parameters for the blockchain and our backend
// we can create and instance of CurrentBlockchainStatus class.
// we are going to do this through a shared pointer. This way we will
// have only once instance of this class, which we can easly inject
// and pass around other class which need to access blockchain data

auto current_bc_status
        = make_shared<xmreg::CurrentBlockchainStatus>(
            bc_setup,
            std::make_unique<xmreg::MicroCore>(),
            std::make_unique<xmreg::RPCCalls>(bc_setup.deamon_url));

// since CurrentBlockchainStatus class monitors current status
// of the blockchain (e.g., current height) .This is the only class
// that has direct access to blockchain and talks (using rpc calls)
// with the monero deamon.
if (!current_bc_status->init_monero_blockchain())
{
    OMERROR << "Error accessing blockchain.";
    return EXIT_FAILURE;
}

// launch the status monitoring thread so that it keeps track of blockchain
// info, e.g., current height. Information from this thread is used
// by tx searching threads that are launched for each user independently,
// when they log back or create new account.

xmreg::ThreadRAII blockchain_monitoring_thread(
            std::thread([current_bc_status]
                        {current_bc_status->monitor_blockchain();}),
            xmreg::ThreadRAII::DtorAction::join);


OMINFO << "Blockchain monitoring thread started";

// try connecting to the mysql
shared_ptr<xmreg::MySqlAccounts> mysql_accounts;

try
{
    // MySqlAccounts will try connecting to the mysql database
    mysql_accounts = make_shared<xmreg::MySqlAccounts>(current_bc_status);

    OMINFO << "Connected to the MySQL";
}
catch(std::exception const& e)
{
    OMERROR << e.what();
    return EXIT_FAILURE;
}

// at this point we should be connected to the mysql

// mysql connection will timeout after few hours
// of iddle time. so we have this tiny helper
// thread to ping mysql, thus keeping it alive.
//
// "A completely different way to tackle this,
// if your program doesn’t block forever waiting on I/O while idle,
// is to periodically call Connection::ping(). [12]
// This sends the smallest possible amount of data to the database server,
// which will reset its idle timer and cause it to respond, so ping() returns true.
// If it returns false instead, you know you need to reconnect to the server.
// Periodic pinging is easiest to do if your program uses asynchronous I/O,
// threads, or some kind of event loop to ensure that you can call
// something periodically even while the rest of the program has nothing to do."
// from: https://tangentsoft.net/mysql++/doc/html/userman/tutorial.html#connopts
//

xmreg::MysqlPing mysql_ping {mysql_accounts->get_connection()};

xmreg::ThreadRAII mysql_ping_thread(
        std::thread(std::ref(mysql_ping)),
        xmreg::ThreadRAII::DtorAction::join);

OMINFO << "MySQL ping thread started";

// create REST JSON API services
xmreg::YourMoneroRequests open_monero(mysql_accounts, current_bc_status);

// create Open Monero APIs
MAKE_RESOURCE(login);
MAKE_RESOURCE(get_address_txs);
MAKE_RESOURCE(get_address_info);
MAKE_RESOURCE(get_unspent_outs);
MAKE_RESOURCE(get_random_outs);
MAKE_RESOURCE(submit_raw_tx);
MAKE_RESOURCE(import_wallet_request);
MAKE_RESOURCE(import_recent_wallet_request);
MAKE_RESOURCE(get_tx);
MAKE_RESOURCE(get_version);

// restbed service
Service service;

// Publish the Open Monero API created so that front end can use it
service.publish(login);
service.publish(get_address_txs);
service.publish(get_address_info);
service.publish(get_unspent_outs);
service.publish(get_random_outs);
service.publish(submit_raw_tx);
service.publish(import_wallet_request);
service.publish(import_recent_wallet_request);
service.publish(get_tx);
service.publish(get_version);

OMINFO << "JSON API endpoints published";

auto settings = make_shared<Settings>();

if (config_json["ssl"]["enable"])
{
    // based on the example provided at
    // https://github.com/Corvusoft/restbed/blob/34187502642144ab9f749ab40f5cdbd8cb17a54a/example/https_service/source/example.cpp
    auto ssl_settings = make_shared<SSLSettings>( );

    Uri ssl_key = Uri(config_json["ssl"]["ssl-key"].get<string>());
    Uri ssl_crt = Uri(config_json["ssl"]["ssl-crt"].get<string>());
    Uri dh_pem  = Uri(config_json["ssl"]["dh-pem"].get<string>());

    ssl_settings->set_http_disabled(true);
    ssl_settings->set_port(app_port);
    ssl_settings->set_private_key(ssl_key);
    ssl_settings->set_certificate(ssl_crt);
    ssl_settings->set_temporary_diffie_hellman(dh_pem);

    settings->set_ssl_settings(ssl_settings);

    // can check using: curl -k -v -w'\n' -X POST 'https://127.0.0.1:1984/get_version'

    OMINFO << "Start the service at https://127.0.0.1:" << app_port;
}
else
{
    settings->set_port(app_port);
    settings->set_default_header("Connection", "close");

    OMINFO << "Start the service at http://127.0.0.1:" << app_port;
}


service.start(settings);

return EXIT_SUCCESS;
}
