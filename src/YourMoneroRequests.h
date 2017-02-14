//
// Created by mwo on 8/12/16.
//

#ifndef RESTBED_XMR_YOURMONEROREQUESTS_H
#define RESTBED_XMR_YOURMONEROREQUESTS_H

#include <iostream>
#include <functional>


#include "CurrentBlockchainStatus.h"
#include "MySqlAccounts.h"

#include "../ext/restbed/source/restbed"

namespace xmreg
{

using namespace std;
using namespace restbed;
using namespace nlohmann;


string
get_current_time(const char* format = "%a, %d %b %Y %H:%M:%S %Z");

multimap<string, string>
make_headers(const multimap<string, string>& extra_headers = multimap<string, string>());

struct handel_
{
    using fetch_func_t = function< void ( const shared_ptr< Session >, const Bytes& ) >;

    fetch_func_t request_callback;

    handel_(const fetch_func_t& callback);

    void operator()(const shared_ptr< Session > session);
};


class YourMoneroRequests
{

    // this manages all mysql queries
   shared_ptr<MySqlAccounts> xmr_accounts;


public:

    static bool show_logs;

    YourMoneroRequests(shared_ptr<MySqlAccounts> _acc);

    /**
     * A login request handler.
     *
     * It takes address and viewkey from the request
     * and check mysql if address/account exist. If yes,
     * it returns this account. If not, it creates new one.
     *
     * Once this complites, a thread is started that looks
     * for txs belonging to that account.
     *
     * @param session a Restbed session
     * @param body a POST body, i.e., json string
     */
    void
    login(const shared_ptr<Session> session, const Bytes & body);

    void
    get_address_txs(const shared_ptr< Session > session, const Bytes & body);

    void
    get_address_info(const shared_ptr< Session > session, const Bytes & body);

    void
    get_unspent_outs(const shared_ptr< Session > session, const Bytes & body);

    void
    get_random_outs(const shared_ptr< Session > session, const Bytes & body);

    void
    submit_raw_tx(const shared_ptr< Session > session, const Bytes & body);

    void
    import_wallet_request(const shared_ptr< Session > session, const Bytes & body);

    shared_ptr<Resource>
    make_resource(function< void (YourMoneroRequests&, const shared_ptr< Session >, const Bytes& ) > handle_func,
                  const string& path);

    static void
    generic_options_handler( const shared_ptr< Session > session );

    static void
    print_json_log(const string& text, const json& j);


    static inline string
    body_to_string(const Bytes & body);

    static inline json
    body_to_json(const Bytes & body);

    inline uint64_t
    get_current_blockchain_height();

private:

   /**
    * Search for our txs in the mempool
    *
    * The method searches for our txs (outputs and inputs)
    * in the mempool. It does basically same what TxSearch class is
    * doing. The difference is that TxSearch class searches in a thread
    * in a blockchain. It does not scan for tx in mempool. This is because
    * TxSearch writes what it finds into database for permament storage.
    * However txs in mempool are not permament. Also since we want to
    * give the end user quick update on incoming/outging tx, this method
    * will be executed whenever frontend wants. By default it is every
    * 10 seconds. TxSearch class is timed independetly of the frontend.
    * Also since we dont write here anything to the database, we
    * return a json that will be appended to json produced by get_address_tx
    * and similar function. The outputs here cant be spent anyway. This is
    * only for end user information. The txs found here will be written
    * to database later on by TxSearch thread when they will be added
    * to the blockchain.
    *
    * @return json
    */
    json
    find_txs_in_mempool(const account_public_address* address,
                        const secret_key* viewkey);
};



}
#endif //RESTBED_XMR_YOURMONEROREQUESTS_H
