#include <iostream>

#include <memory>
#include <cstdlib>

#include "ext/restbed/source/restbed"

#include "src/MySqlConnector.h"
#include "src/YourMoneroRequests.h"
#include "src/tools.h"
#include "src/TxSearch.h"

using namespace std;
using namespace restbed;
using namespace nlohmann;


static inline string
body_to_string(const Bytes & body)
{
    return string(reinterpret_cast<const char *>(body.data()), body.size());
}

static inline json
body_to_json(const Bytes & body)
{
    json j = json::parse(body_to_string(body));
    return j;
}

static void
print_json_log(const string& text, const json& j)
{
    cout << text << '\n' << j.dump(4) << endl;
}

// map that will keep track of search threads. In the
// map, key is address to which a running thread belongs to.
map<string, shared_ptr<xmreg::TxSearch>> searching_threads;

bool
start_tx_search_thread(xmreg::XmrAccount& acc)
{
    if (searching_threads.count(acc.address) > 0)
    {
        // thread for this address exist, dont make new one
        cout << "Thread exisist, dont make new one" << endl;
        return false;
    }

    // make a tx_search object for the given xmr account
    searching_threads[acc.address] = make_shared<xmreg::TxSearch>(acc);

    // start the thread for the created object
    std::thread t1 {&xmreg::TxSearch::search, searching_threads[acc.address].get()};
    t1.detach();

    return true;
}

void
login2(const shared_ptr< Session > session)
{

    const auto request = session->get_request( );

    int content_length = request->get_header( "Content-Length", 0);

    session->fetch( content_length, [](const shared_ptr< Session > session, const Bytes & body )
    {

        xmreg::MySqlAccounts xmr_accounts;

        json j_request = body_to_json(body);

        if (true)
            print_json_log("login request: ", j_request);

        string xmr_address  = j_request["address"];

        // a placeholder for exciting or new account data
        xmreg::XmrAccount acc;

        uint64_t acc_id {0};

        json j_response;

        // select this account if its existing one
        if (xmr_accounts.select(xmr_address, acc))
        {
            j_response = {{"new_address", false}};
        }
        else
        {
            // account does not exist, so create new one
            // for this address

            if ((acc_id = xmr_accounts.create(xmr_address)) != 0)
            {
                // select newly created account
                if (xmr_accounts.select(acc_id, acc))
                {
                    j_response = {{"new_address", true}};
                }
            }
        }

        cout << acc << endl;

        // so we have an account now. Either existing or
        // newly created. Thus, we can start a tread
        // which will scan for transactions belonging to
        // that account, using its address and view key.
        // the thread will scan the blockchain for txs belonging
        // to that account and updated mysql database whenever it
        // will find something.
        //
        // The other client (i.e., a webbrowser) will query other functions to retrieve
        // any belonging transactions in a loop. Thus the thread does not need
        // to do anything except looking for tx and updating mysql
        // with relative tx information

        if (start_tx_search_thread(acc))
        {
            cout << "Search thread started" << endl;
        }

        string response_body = j_response.dump();

        auto response_headers = xmreg::make_headers({{ "Content-Length", to_string(response_body.size())}});

        session->close( OK, response_body, response_headers);
    } );



}



int
main()
{
//    xmreg::MySqlAccounts xmr_accounts;
//
//    //xmr_accounts.create_account("41vEA7Ye8Bpeda6g59v5t46koWrVn2PNgEKgzquJjmiKCFTsh9gajr8J3pad49rqu581TAtFGCH9CYTCkYrCpuWUG9GkgeB");
//
//
//    string addr = "41vEA7Ye8Bpeda6g9v5t46koWrVn2PNgEKgluJjmiKCFTsh9gajr8J3pad49rqu581TAtFGCH9CYTCkYrCpuWUG9GkgeB";
//
//    xmreg::XmrAccount acc;
//
//    bool r = xmr_accounts.select(addr, acc);
//
//    if (r)
//    {
//        cout << "Account foudn: " << acc.id << endl;
//
//        xmreg::XmrAccount new_acc = acc;
//        new_acc.total_received = 20e12;
//
//        cout << xmr_accounts.update(acc, new_acc) << endl;
//
//    }
//    else
//    {
//        cout << "Account does not exist" << endl;
//        cout << xmr_accounts.create(addr) << endl;
//    }


//   xmreg::MySqlAccounts xmr_accounts;
////
//    string addr = "41vEA7Ye8Bpeda6g9v5t46koWrVn2PNgEKgluJjmiKCFTsh9gajr8J3pad49rqu581TAtFGCH9CYTCkYrCpuWUG9GkgeB";
//
//    xmreg::XmrAccount acc;
//
//    cout << xmr_accounts.select(addr, acc) << endl;
//
//    cout << xmr_accounts.select("fdfdfd", acc) << endl;
//
//    cout << xmr_accounts.select(addr, acc) << endl;

    xmreg::YourMoneroRequests::show_logs = true;

    xmreg::YourMoneroRequests your_xmr(
            shared_ptr<xmreg::MySqlAccounts>(new xmreg::MySqlAccounts{}));

    //your_xmr.start_tx_search_thread(acc);

//    auto login                 = your_xmr.make_resource(
//            &xmreg::YourMoneroRequests::login                , "/login");


//    auto login = make_shared< Resource >( );
//    login->set_path( "/login" );
//    login->set_method_handler( "POST",
//                                  std::bind(&xmreg::YourMoneroRequests::login2,
//                                            your_xmr,
//                                            std::placeholders::_1)
//    );
//    login->set_method_handler( "OPTIONS", &xmreg::YourMoneroRequests::generic_options_handler);



    auto login = make_shared< Resource >( );
    login->set_path( "/login" );
    login->set_method_handler( "POST", &login2);
    login->set_method_handler( "OPTIONS", &xmreg::YourMoneroRequests::generic_options_handler);


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