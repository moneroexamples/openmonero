#include <iostream>

#include <memory>
#include <cstdlib>

#include "ext/restbed/source/restbed"

using namespace std;
using namespace restbed;


void
post_method_handler( const shared_ptr< Session > session )
{
    const auto request = session->get_request( );

    size_t content_length = request->get_header( "Content-Length", 0);

    session->fetch(content_length, [ ]( const shared_ptr< Session > session, const Bytes & body )
    {
        string _body(reinterpret_cast<const char *>(body.data()), body.size());

        cout << _body << endl;

        string response_body {"test output"};

        session->close( OK, response_body, {
                { "Content-Length", to_string(response_body.size()) },
                { "Content-Type", "application/json" }
        } );
    } );
}

int
main()
{

    auto resource = make_shared< Resource >( );
    resource->set_path( "/resource" );
    resource->set_method_handler( "POST", post_method_handler );


    // SSL based on this
    // https://github.com/Corvusoft/restbed/blob/34187502642144ab9f749ab40f5cdbd8cb17a54a/example/https_service/source/example.cpp
    auto ssl_settings = make_shared< SSLSettings >( );
    ssl_settings->set_http_disabled( true );
    ssl_settings->set_port( 1984 );
    ssl_settings->set_private_key( Uri( "file:///tmp/server.key" ) );
    ssl_settings->set_certificate( Uri( "file:///tmp/server.crt" ) );
    ssl_settings->set_temporary_diffie_hellman( Uri( "file:///tmp/dh768.pem" ) );


    auto settings = make_shared< Settings >( );
    settings->set_ssl_settings( ssl_settings );

    settings->set_default_header( "Connection", "close" );

    Service service;
    service.publish( resource );
    service.start( settings );

    return EXIT_SUCCESS;

    std::cout << "Hello, World!" << std::endl;
    return 0;
}