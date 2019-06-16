#include "src/om_log.h"
#include "src/CmdLineOptions.h"
#include "src/MicroCore.h"
#include "src/OpenMoneroRequests.h"
#include "src/ThreadRAII.h"
#include "src/db/MysqlPing.h"

#include <iostream>
#include <memory>
#include <cstdlib>


using namespace std;
using namespace restbed;

using boost::filesystem::path;

// signal exit handler, addpated from aleth
class ExitHandler
{
public:
    static std::mutex m;
    static std::condition_variable cv;

    static void exitHandler(int)
    {
        std::lock_guard<std::mutex> lk(m);
        s_shouldExit = true;
        OMINFO << "Request to finish the openmonero received";
        cv.notify_one();
    }

    bool shouldExit() const { return s_shouldExit; }

private:
    static bool s_shouldExit;
};

bool ExitHandler::s_shouldExit {false};
std::mutex ExitHandler::m;
std::condition_variable ExitHandler::cv;



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

auto monero_log_level  =
        *(opts.get_option<size_t>("monero-log-level"));

auto verbose_level = 
        *(opts.get_option<size_t>("verbose"));

if (monero_log_level < 1 || monero_log_level > 4)
{
    cerr << "monero-log-level,m option must be between 1 and 4!\n";
    return EXIT_SUCCESS;
}

if (verbose_level < 0 || verbose_level > 4)
{
    cerr << "verbose,v option must be between 0 and 4!\n";
    return EXIT_SUCCESS;
}

// setup monero logger
mlog_configure(mlog_get_default_log_path(""), true);
mlog_set_log(std::to_string(monero_log_level).c_str());

auto log_file  = *(opts.get_option<string>("log-file"));

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

// default format: %datetime %level [%logger] %msg
// we change to add file and func
defaultConf.setGlobally(el::ConfigurationType::Format,
                        "%datetime [%levshort,%logger,%fbase:%func:%line]"
                        " %msg");

el::Loggers::setVerboseLevel(verbose_level);

el::Loggers::reconfigureLogger(OPENMONERO_LOG_CATEGORY, defaultConf);

OMINFO << "OpenMonero is starting";

if (verbose_level > 0)
    OMINFO << "Using verbose log level to: " << verbose_level;

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
auto app_port   = boost::lexical_cast<uint16_t>(*port_opt);

// set mysql/mariadb connection details
xmreg::MySqlConnector::url      = config_json["database"]["url"];
xmreg::MySqlConnector::port     = config_json["database"]["port"];
xmreg::MySqlConnector::username = config_json["database"]["user"];
xmreg::MySqlConnector::password = config_json["database"]["password"];
xmreg::MySqlConnector::dbname   = config_json["database"]["dbname"];

// number of thread in blockchain access pool thread
auto threads_no = std::max<uint32_t>(
        std::thread::hardware_concurrency()/2, 2u) - 1;

if (bc_setup.blockchain_treadpool_size > 0)
    threads_no = bc_setup.blockchain_treadpool_size;

if (threads_no > 100)
{
    threads_no = 100;
    OMWARN << "Requested Thread Pool size " 
        << threads_no << " is greater than 100!."
            " Overwriting to 100!" ;
}

OMINFO << "Thread pool size: " << threads_no << " threads";

// once we have all the parameters for the blockchain and our backend
// we can create and instance of CurrentBlockchainStatus class.
// we are going to do this through a shared pointer. This way we will
// have only once instance of this class, which we can easly inject
// and pass around other class which need to access blockchain data

auto current_bc_status
        = make_shared<xmreg::CurrentBlockchainStatus>(
            bc_setup,
            std::make_unique<xmreg::MicroCore>(),
            std::make_unique<xmreg::RPCCalls>(bc_setup.deamon_url),
            std::make_unique<TP::ThreadPool>(threads_no));

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

std::thread blockchain_monitoring_thread(
            [&current_bc_status]()
{
    current_bc_status->monitor_blockchain();
});


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

xmreg::MysqlPing mysql_ping {
        mysql_accounts->get_connection(),
        bc_setup.mysql_ping_every};

std::thread mysql_ping_thread(
            [&mysql_ping]()
{
    mysql_ping();
});


OMINFO << "MySQL ping thread started";

// create REST JSON API services
xmreg::OpenMoneroRequests open_monero(mysql_accounts, current_bc_status);

// create Open Monero APIs
MAKE_RESOURCE(login);
MAKE_RESOURCE(ping);
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
service.publish(ping);
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

// intercept basic termination requests,
// including Ctrl+c
ExitHandler exitHandler;

signal(SIGABRT, ExitHandler::exitHandler);
signal(SIGTERM, ExitHandler::exitHandler);
signal(SIGINT, ExitHandler::exitHandler);


// main restbed thread. this is where
// restbed will be running and handling
// requests
std::thread restbed_service(
            [&service, &settings]()
{
    OMINFO << "Starting restbed service thread.";
    service.start(settings);
});


// we are going to whait here for as long
// as control+c hasn't been pressed
{
    std::unique_lock<std::mutex> lk(ExitHandler::m);
    ExitHandler::cv.wait(lk, [&exitHandler]{
        return exitHandler.shouldExit();});
}


//////////////////////////////////////////////
// Try to gracefully stop all threads/services
//////////////////////////////////////////////

OMINFO << "Stopping restbed service.";
service.stop();
restbed_service.join();

OMINFO << "Stopping blockchain_monitoring_thread. Please wait.";
current_bc_status->stop();
blockchain_monitoring_thread.join();

OMINFO << "Stopping mysql_ping. Please wait.";
mysql_ping.stop();
mysql_ping_thread.join();

OMINFO << "Disconnecting from database.";
mysql_accounts->disconnect();

OMINFO << "All done. Bye.";

return EXIT_SUCCESS;
}
