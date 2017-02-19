//
// Created by mwo on 8/01/17.
//

#define MYSQLPP_SSQLS_NO_STATICS 1

#include "YourMoneroRequests.h"

#include "ssqlses.h"
#include "OutputInputIdentification.h"

namespace xmreg
{


string
get_current_time(const char* format)
{

    auto current_time = date::make_zoned(
            date::current_zone(),
            date::floor<chrono::seconds>(std::chrono::system_clock::now())
    );

    return date::format(format, current_time);
}


multimap<string, string>
make_headers(const multimap<string, string>& extra_headers)
{
    multimap<string, string> headers {
            {"Date", get_current_time()},
            {"Access-Control-Allow-Origin",      "http://127.0.0.1:81"},
            {"access-control-allow-headers",     "*, DNT,X-CustomHeader,Keep-Alive,User-Agent,X-Requested-With,If-Modified-Since,Cache-Control,Content-Type,Set-Cookie"},
            {"access-control-max-age",           "86400, 1728000"},
            {"access-control-allow-methods",     "GET, POST, OPTIONS"},
            {"access-control-allow-credentials", "true"},
            {"Content-Type",                     "application/json"}
    };

    headers.insert(extra_headers.begin(), extra_headers.end());

    return headers;
};


handel_::handel_(const fetch_func_t& callback):
        request_callback {callback}
{}

void
handel_::operator()(const shared_ptr< Session > session)
{
    const auto request = session->get_request( );
    size_t content_length = request->get_header( "Content-Length", 0);
    session->fetch(content_length, this->request_callback);
}



YourMoneroRequests::YourMoneroRequests(shared_ptr<MySqlAccounts> _acc):
    xmr_accounts {_acc}
{}


void
YourMoneroRequests::login(const shared_ptr<Session> session, const Bytes & body)
{
    json j_request = body_to_json(body);

//    if (show_logs)
//        print_json_log("login request: ", j_request);

    string xmr_address  = j_request["address"];

    // a placeholder for exciting or new account data
    XmrAccount acc;

    uint64_t acc_id {0};

    json j_response;

    // select this account if its existing one
    if (xmr_accounts->select(xmr_address, acc))
    {
        j_response = {{"new_address", false}};
    }
    else
    {
        // account does not exist, so create new one
        // for this address

        // we will save current blockchain height
        // in mysql, so that we know from what block
        // to start searching txs of this new acount
        // make it 1 block lower than current, just in case.
        // this variable will be our using to initialize
        // `canned_block_height` in mysql Accounts table.
        uint64_t current_blkchain_height = get_current_blockchain_height() - 1;

        if ((acc_id = xmr_accounts->insert(xmr_address, current_blkchain_height)) != 0)
        {
            // select newly created account
            if (xmr_accounts->select(acc_id, acc))
            {
                j_response = {{"new_address", true}};
            }
        }
    }

    acc.viewkey = j_request["view_key"];

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

    if (CurrentBlockchainStatus::start_tx_search_thread(acc))
    {
        cout << "Search thread started" << endl;
    }

    string response_body = j_response.dump();

    auto response_headers = make_headers({{ "Content-Length", to_string(response_body.size())}});

    session->close( OK, response_body, response_headers);
}

void
YourMoneroRequests::get_address_txs(const shared_ptr< Session > session, const Bytes & body)
{
    json j_request = body_to_json(body);

//        if (show_logs)
//            print_json_log("get_address_txs request: ", j_request);

    string xmr_address  = j_request["address"];

    // initialize json response
    json j_response {
            { "total_received", "0"},           // calculated in this function
            { "total_received_unlocked", "0"},  // calculated in this function
            { "scanned_height", 0},       // not used. it is here to match mymonero
            { "scanned_block_height", 0}, // taken from Accounts table
            { "start_height", 0},         // blockchain hieght when acc was created
            { "transaction_height", 0},   // not used. it is here to match mymonero
            { "blockchain_height", 0},    // current blockchain height
            { "transactions", json::array()}
    };


    // a placeholder for exciting or new account data
    xmreg::XmrAccount acc;

    // select this account if its existing one
    if (xmr_accounts->select(xmr_address, acc)) {

        uint64_t total_received {0};
        uint64_t total_received_unlocked {0};

        j_response["total_received"] = total_received;
        j_response["start_height"] = acc.start_height;
        j_response["scanned_block_height"] = acc.scanned_block_height;
        j_response["blockchain_height"] = CurrentBlockchainStatus::get_current_blockchain_height();

        vector<XmrTransaction> txs;

        if (xmr_accounts->select_txs_for_account_spendability_check(acc.id, txs))
        {

            json j_txs = json::array();

            for (XmrTransaction tx: txs)
            {
                json j_tx = tx.to_json();

                // mark that this is from blockchain.
                // tx stored in mysql/mariadb are only from blockchain
                // and never from mempool.
                j_tx["mempool"] = false;

                vector<XmrInput> inputs;

                if (xmr_accounts->select_inputs_for_tx(tx.id, inputs))
                {
                    json j_spent_outputs = json::array();

                    uint64_t total_spent {0};

                    for (XmrInput input: inputs)
                    {
                        XmrOutput out;

                        if (xmr_accounts->select_output_with_id(input.output_id, out))
                        {
                            total_spent += input.amount;

                            j_spent_outputs.push_back({
                                    {"amount"     , input.amount},
                                    {"key_image"  , input.key_image},
                                    {"tx_pub_key" , out.tx_pub_key},
                                    {"out_index"  , out.out_index},
                                    {"mixin"      , out.mixin}});

                        }
                    }

                    j_tx["total_sent"] = total_spent;

                    j_tx["spent_outputs"] = j_spent_outputs;

                } // if (xmr_accounts->select_inputs_for_tx(tx.id, inputs))

                total_received += tx.total_received;

                if (bool {tx.spendable})
                {
                    total_received_unlocked += tx.total_received;
                }

                j_txs.push_back(j_tx);

            } // for (XmrTransaction tx: txs)

            j_response["total_received"]          = total_received;
            j_response["total_received_unlocked"] = total_received_unlocked;

            j_response["transactions"] = j_txs;

        } // if (xmr_accounts->select_txs_for_account_spendability_check(acc.id, txs))

    } //  if (xmr_accounts->select(xmr_address, acc))


    // append txs found in mempool to the json returned

    json j_mempool_tx;

    if (CurrentBlockchainStatus::find_txs_in_mempool(
            xmr_address, j_mempool_tx))
    {
        if(!j_mempool_tx.empty())
        {
            uint64_t total_received_mempool {0};
            uint64_t total_sent_mempool {0};

            // get last tx id (i.e., index) so that we can
            // set some ids for the mempool txs. These ids are
            // used for sorting in the frontend. Since we want mempool
            // tx to be first, they need to be higher than last_tx_id_db
            uint64_t last_tx_id_db {0};

            if (!j_response["transactions"].empty())
            {
                last_tx_id_db = j_response["transactions"].back()["id"];
            }

            for (json& j_tx: j_mempool_tx)
            {
                cout    << "mempool j_tx[\"total_received\"]: "
                        << j_tx["total_received"] << endl;

                j_tx["id"] = ++last_tx_id_db;

                total_received_mempool += j_tx["total_received"].get<uint64_t>();
                total_sent_mempool     += j_tx["total_sent"].get<uint64_t>();

                j_response["transactions"].push_back(j_tx);
            }

            j_response["total_received"] = j_response["total_received"].get<uint64_t>()
                                           + total_received_mempool;
        }

    }

    string response_body = j_response.dump();

    auto response_headers = make_headers({{ "Content-Length", to_string(response_body.size())}});

    session->close(OK, response_body, response_headers);
}

void
YourMoneroRequests::get_address_info(const shared_ptr< Session > session, const Bytes & body)
{
    json j_request = body_to_json(body);

//        if (show_logs)
//            print_json_log("get_address_info request: ", j_request);

    string xmr_address  = j_request["address"];

    json j_response  {
            {"locked_funds", "0"},       // xmr in mempool transactions
            {"total_received", "0"},     // taken from Accounts table
            {"total_sent", "0"},         // sum of xmr in possible spent outputs
            {"scanned_height", 0},       // not used. it is here to match mymonero
            {"scanned_block_height", 0}, // taken from Accounts table
            {"start_height", 0},         // not used, but available in Accounts table.
            // it is here to match mymonero
            {"transaction_height", 0},   // not used. it is here to match mymonero
            {"blockchain_height", 0},    // current blockchain height
            {"spent_outputs", nullptr}   // list of spent outputs that we think
            // user has spent. client side will
            // filter out false positives since
            // only client has spent key
    };


    // a placeholder for exciting or new account data
    xmreg::XmrAccount acc;

    // select this account if its existing one
    if (xmr_accounts->select(xmr_address, acc))
    {
        uint64_t total_received {0};

        // ping the search thread that we still need it.
        // otherwise it will finish after some time.
        CurrentBlockchainStatus::ping_search_thread(xmr_address);

        j_response["total_received"]       = total_received;
        j_response["start_height"]         = acc.start_height;
        j_response["scanned_block_height"] = acc.scanned_block_height;
        j_response["blockchain_height"]    = CurrentBlockchainStatus::get_current_blockchain_height();

        uint64_t total_sent {0};

        vector<XmrTransaction> txs;

        if (xmr_accounts->select_txs_for_account_spendability_check(acc.id, txs))
        {
            json j_spent_outputs = json::array();

            for (XmrTransaction tx: txs)
            {
                vector<XmrOutput> outs;

                if (xmr_accounts->select_outputs_for_tx(tx.id, outs))
                {
                    for (XmrOutput &out: outs)
                    {
                        // check if the output, has been spend
                        vector<XmrInput> ins;

                        if (xmr_accounts->select_inputs_for_out(out.id, ins))
                        {
                            for (XmrInput& in: ins)
                            {
                                j_spent_outputs.push_back({
                                    {"amount"     , in.amount},
                                    {"key_image"  , in.key_image},
                                    {"tx_pub_key" , out.tx_pub_key},
                                    {"out_index"  , out.out_index},
                                    {"mixin"      , out.mixin},
                                });

                                total_sent += in.amount;
                            }
                        }

                        total_received += out.amount;
                    }
                }
            }

            j_response["total_received"] = total_received;
            j_response["total_sent"]     = total_sent;

            j_response["spent_outputs"]  = j_spent_outputs;
        }

    } // if (xmr_accounts->select(xmr_address, acc))

    string response_body = j_response.dump();

    auto response_headers = make_headers({{ "Content-Length", to_string(response_body.size())}});

    session->close( OK, response_body, response_headers);
}


void
YourMoneroRequests::get_unspent_outs(const shared_ptr< Session > session, const Bytes & body)
{
    json j_request = body_to_json(body);

//        if (show_logs)
//            print_json_log("get_unspent_outs request: ", j_request);

    string xmr_address      = j_request["address"];
    uint64_t mixin          = j_request["mixin"];
    bool use_dust           = j_request["use_dust"];

    uint64_t dust_threshold = boost::lexical_cast<uint64_t>(j_request["dust_threshold"].get<string>());
    uint64_t amount         = boost::lexical_cast<uint64_t>(j_request["amount"].get<string>());

    json j_response  {
            {"amount" , 0},            // total value of the outputs
            {"outputs", json::array()} // list of outputs
            // exclude those without require
            // no of confirmation
    };

    // a placeholder for exciting or new account data
    xmreg::XmrAccount acc;

    // select this account if its existing one
    if (xmr_accounts->select(xmr_address, acc))
    {
        uint64_t total_outputs_amount {0};

        uint64_t current_blockchain_height
                = CurrentBlockchainStatus::get_current_blockchain_height();

        vector<XmrTransaction> txs;

        // retrieve txs from mysql associated with the given address
        if (xmr_accounts->select_txs(acc.id, txs))
        {
            // we found some txs.

            json& j_outputs = j_response["outputs"];

            for (XmrTransaction& tx: txs)
            {
                // we skip over locked outputs
                // as they cant be spent anyway.
                // thus no reason to return them to the frontend
                // for constructing a tx.

                if (!CurrentBlockchainStatus::is_tx_unlocked(tx.unlock_time, tx.height))
                {
                    continue;
                }

                vector<XmrOutput> outs;

                if (xmr_accounts->select_outputs_for_tx(tx.id, outs))
                {
                    for (XmrOutput &out: outs)
                    {
                        // skip outputs considered as dust
                        if (out.amount < dust_threshold)
                        {
                            continue;
                        }

                        // need to check for rct commintment
                        // coinbase ringct txs dont have
                        // rct filed in them. Thus
                        // we need to make them.

                        uint64_t global_amount_index = out.global_index;

                        uint64_t out_amount {out.amount};

                        string rct = out.get_rct();

                        // but if ringct tx, set it amount to zero
                        // as in Outputs table we store decoded outputs amounts
                        if (tx.is_rct)
                        {
                            // out_amount = 0;

                            if (tx.coinbase)
                            {
                                // not really sure how to treet coinbase
                                // ringct unspent txs.
                                // // https://github.com/monero-project/monero/blob/eacf2124b6822d088199179b18d4587404408e0f/src/wallet/wallet2.cpp#L893

                                output_data_t od =
                                        CurrentBlockchainStatus::get_output_key(
                                                0, global_amount_index);

                                string rtc_outpk =  pod_to_hex(od.commitment);
                                string rtc_mask  =  pod_to_hex(rct::identity());
                                string rtc_amount(64, '0');

                                rct = rtc_outpk + rtc_mask + rtc_amount;
                            }
                        }

//                        tuple<string, string, string>
//                                rct_field = CurrentBlockchainStatus::construct_output_rct_field(
//                                global_amount_index, out_amount);
//
//                        string rct =  std::get<0>(rct_field)    // rct_pk
//                                      + std::get<1>(rct_field)  // rct_mask
//                                      + std::get<2>(rct_field); // rct_amount



                        //string rct = out.get_rct();

                        json j_out{
                                {"amount",           out.amount},
                                {"public_key",       out.out_pub_key},
                                {"index",            out.out_index},
                                {"global_index",     out.global_index},
                                {"rct"         ,     rct},
                                {"tx_id",            out.tx_id},
                                {"tx_hash",          tx.hash},
                                {"tx_prefix_hash",   tx.prefix_hash},
                                {"tx_pub_key"    ,   out.tx_pub_key},
                                {"timestamp",        out.timestamp},
                                {"height",           tx.height},
                                {"spend_key_images", json::array()}
                        };

                        vector<XmrInput> ins;

                        if (xmr_accounts->select_inputs_for_out(out.id, ins))
                        {
                            json& j_ins = j_out["spend_key_images"];

                            for (XmrInput& in: ins)
                            {
                                j_ins.push_back(in.key_image);
                            }
                        }

                        j_outputs.push_back(j_out);

                        total_outputs_amount += out.amount;

                    }  //for (XmrOutput &out: outs)

                } // if (xmr_accounts->select_outputs_for_tx(tx.id, outs))

            } // for (XmrTransaction& tx: txs)

        } //  if (xmr_accounts->select_txs(acc.id, txs))

        j_response["amount"] = total_outputs_amount;


        // need proper per_kb_fee estimate for testnet as
        // it is already using dynanamic fees. frontend
        // uses old fixed fees.
        if (CurrentBlockchainStatus::testnet)
        {
            uint64_t fee_estimated {DYNAMIC_FEE_PER_KB_BASE_FEE};

            if (CurrentBlockchainStatus::get_dynamic_per_kb_fee_estimate(fee_estimated))
            {
                j_response["per_kb_fee"] = fee_estimated;
            }
        }

    } //  if (xmr_accounts->select(xmr_address, acc))

    string response_body = j_response.dump();

    auto response_headers = make_headers({{ "Content-Length", to_string(response_body.size())}});

    session->close( OK, response_body, response_headers);
}

void
YourMoneroRequests::get_random_outs(const shared_ptr< Session > session, const Bytes & body)
{
    json j_request = body_to_json(body);

//        if (show_logs)
//            print_json_log("get_unspent_outs request: ", j_request);

    uint64_t count     = j_request["count"];
    vector<uint64_t> amounts;

    for (json amount: j_request["amounts"])
    {
        amounts.push_back(boost::lexical_cast<uint64_t>(amount.get<string>()));
        //amounts.push_back(0);
    }

    json j_response  {
            {"amount_outs", json::array()}
    };

    vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount> found_outputs;


    if (CurrentBlockchainStatus::get_random_outputs(amounts, count, found_outputs))
    {
        json& j_amount_outs = j_response["amount_outs"];

        for (const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& outs: found_outputs)
        {
            json j_outs {{"amount", outs.amount},
                         {"outputs", json::array()}};


            json& j_outputs = j_outs["outputs"];

            for (const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry& out: outs.outs)
            {
                uint64_t global_amount_index = out.global_amount_index;

                tuple<string, string, string>
                        rct_field = CurrentBlockchainStatus::construct_output_rct_field(
                                    global_amount_index, outs.amount);

                json out_details {
                        {"global_index", out.global_amount_index},
                        {"public_key"  , pod_to_hex(out.out_key)},
                        {"rct"         , std::get<0>(rct_field)    // rct_pk
                                         + std::get<1>(rct_field)  // rct_mask
                                         + std::get<2>(rct_field)} // rct_amount
                };

                j_outputs.push_back(out_details);

            } // for (const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry& out: outs.outs)

            j_amount_outs.push_back(j_outs);

        } // for (const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& outs: found_outputs)

    } // if (CurrentBlockchainStatus::get_random_outputs(amounts, count, found_outputs))

    string response_body = j_response.dump();

    auto response_headers = make_headers({{ "Content-Length", to_string(response_body.size())}});

    session->close( OK, response_body, response_headers);
}


void
YourMoneroRequests::submit_raw_tx(const shared_ptr< Session > session, const Bytes & body)
{
    json j_request = body_to_json(body);

//        if (show_logs)
//            print_json_log("get_unspent_outs request: ", j_request);

    string raw_tx_blob     = j_request["tx"];

    json j_response;

    string error_msg;

    if (!CurrentBlockchainStatus::commit_tx(raw_tx_blob, error_msg))
    {
        j_response["status"] = "error";
        j_response["error"] = error_msg;
    }
    else
    {
        j_response["status"] = "success";
    }

    string response_body = j_response.dump();

    auto response_headers = make_headers({{ "Content-Length", to_string(response_body.size())}});

    session->close( OK, response_body, response_headers);
}

void
YourMoneroRequests::import_wallet_request(const shared_ptr< Session > session, const Bytes & body)
{
    json j_request = body_to_json(body);

    if (show_logs)
        print_json_log("import_wallet_request request: ", j_request);

    string xmr_address = j_request["address"];

    // a placeholder for exciting or new account data
    xmreg::XmrPayment xmr_payment;

    json j_response;

    // select this payment if its existing one
    if (xmr_accounts->select_payment_by_address(xmr_address, xmr_payment))
    {
        // payment record exists, so now we need to check if
        // actually payment has been done, and updated
        // mysql record accordingly.

        bool request_fulfilled = bool {xmr_payment.request_fulfilled};

        j_response["payment_id"]        = xmr_payment.payment_id;
        j_response["import_fee"]        = xmr_payment.import_fee;
        j_response["new_request"]       = false;
        j_response["request_fulfilled"] = request_fulfilled;
        j_response["payment_address"]   = xmr_payment.payment_address;
        j_response["status"]            = "Payment not yet received";

        string tx_hash_with_payment;

        if (!request_fulfilled
            && CurrentBlockchainStatus::search_if_payment_made(
                xmr_payment.payment_id,
                xmr_payment.import_fee,
                tx_hash_with_payment))
        {
            XmrPayment updated_xmr_payment = xmr_payment;

            // updated values
            updated_xmr_payment.request_fulfilled = true;
            updated_xmr_payment.tx_hash           = tx_hash_with_payment;

            // save to mysql
            if (xmr_accounts->update_payment(xmr_payment, updated_xmr_payment))
            {

                // set scanned_block_height	to 0 to begin
                // scanning entire blockchain

                XmrAccount acc;

                if (xmr_accounts->select(xmr_address, acc))
                {
                    XmrAccount updated_acc = acc;

                    updated_acc.scanned_block_height = 0;

                    if (xmr_accounts->update(acc, updated_acc))
                    {
                        // if success, set acc to updated_acc;
                        request_fulfilled = true;

                        // change search blk number in the search thread
                        if (!CurrentBlockchainStatus::set_new_searched_blk_no(xmr_address, 0))
                        {
                            cerr << "Updating searched_blk_no failed!" << endl;
                            j_response["status"] = "Updating searched_blk_no failed!";
                        }
                    }
                }
                else
                {
                    cerr << "Updating accounts due to made payment mysql failed! " << endl;
                    j_response["status"] = "Updating accounts due to made payment mysql failed!";
                }
            }
            else
            {
                cerr << "Updating payment mysql failed! " << endl;
                j_response["status"] = "Updating payment mysql failed!";
            }
        }

        if (request_fulfilled)
        {
            j_response["request_fulfilled"]  = request_fulfilled;
            j_response["status"]             = "Payment received. Thank you.";
        }
    }
    else
    {
        // payment request is now, so create its entry in
        // Payments table

        uint64_t payment_table_id {0};

        xmr_payment.address           = xmr_address;
        xmr_payment.payment_id        = pod_to_hex(generated_payment_id());
        xmr_payment.import_fee        = CurrentBlockchainStatus::import_fee; // xmr
        xmr_payment.request_fulfilled = false;
        xmr_payment.tx_hash           = ""; // no tx_hash yet with the payment
        xmr_payment.payment_address   = CurrentBlockchainStatus::import_payment_address;


        if ((payment_table_id = xmr_accounts->insert_payment(xmr_payment)) != 0)
        {
            // payment entry created

            j_response["payment_id"]  = xmr_payment.payment_id;
            j_response["import_fee"]  = xmr_payment.import_fee;
            j_response["new_request"] = true;
            j_response["request_fulfilled"] = bool {xmr_payment.request_fulfilled};
            j_response["payment_address"] = xmr_payment.payment_address;
            j_response["status"] = "Payment not yet received";
        }
    }

    string response_body = j_response.dump();

    auto response_headers = make_headers({{ "Content-Length", to_string(response_body.size())}});

    session->close( OK, response_body, response_headers);
}

shared_ptr<Resource>
YourMoneroRequests::make_resource(
        function< void (YourMoneroRequests&, const shared_ptr< Session >, const Bytes& ) > handle_func,
        const string& path)
{
    auto a_request = std::bind(handle_func, *this, std::placeholders::_1, std::placeholders::_2);

    shared_ptr<Resource> resource_ptr = make_shared<Resource>();

    resource_ptr->set_path(path);
    resource_ptr->set_method_handler( "OPTIONS", generic_options_handler);
    resource_ptr->set_method_handler( "POST"   , handel_(a_request) );

    return resource_ptr;
}


void
YourMoneroRequests::generic_options_handler( const shared_ptr< Session > session )
{
    const auto request = session->get_request( );

    size_t content_length = request->get_header( "Content-Length", 0);

    //cout << "generic_options_handler" << endl;

    session->fetch(content_length, [](const shared_ptr< Session > session, const Bytes & body)
    {
        session->close( OK, string{}, make_headers());
    });
}

void
YourMoneroRequests::print_json_log(const string& text, const json& j)
{
    cout << text << '\n' << j.dump(4) << endl;
}


string
YourMoneroRequests::body_to_string(const Bytes & body)
{
    return string(reinterpret_cast<const char *>(body.data()), body.size());
}

json
YourMoneroRequests::body_to_json(const Bytes & body)
{
    json j = json::parse(body_to_string(body));
    return j;
}


uint64_t
YourMoneroRequests::get_current_blockchain_height()
{
    return CurrentBlockchainStatus::get_current_blockchain_height();
}


// define static variables
bool YourMoneroRequests::show_logs = false;
}



