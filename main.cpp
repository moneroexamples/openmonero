#include <iostream>

#include <memory>
#include <cstdlib>

#include "ext/restbed/source/restbed"

#include "src/MySqlConnector.h"
#include "src/YourMoneroRequests.h"


using namespace std;
using namespace restbed;

int
main()
{
    xmreg::MySqlConnector xmr_accounts;

    //xmr_accounts.create_account("41vEA7Ye8Bpeda6g59v5t46koWrVn2PNgEKgzquJjmiKCFTsh9gajr8J3pad49rqu581TAtFGCH9CYTCkYrCpuWUG9GkgeB");


    //xmr_accounts.select_account("41vEA7Ye8Bpeda6g59v5t46koWrVn2PNgEKgzquJjmiKCFTsh9gajr8J3pad49rqu581TAtFGCH9CYTCkYrCpuWUG9GkgeB");



    xmreg::YourMoneroRequests::show_logs = true;

    xmreg::YourMoneroRequests your_xmr;

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