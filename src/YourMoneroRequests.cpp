//
// Created by mwo on 8/01/17.
//

#define MYSQLPP_SSQLS_NO_STATICS 1


#include "YourMoneroRequests.h"

#include "ssqlses.h"
#include "OutputInputIdentification.h"

namespace xmreg
{


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
{

    // mysql connection will timeout after few hours
    // of iddle time. so we have this tiny helper
    // thread to ping mysql, thus keeping it alive
    xmr_accounts->launch_mysql_pinging_thread();
}


void
YourMoneroRequests::login(const shared_ptr<Session> session, const Bytes & body)
{
    json j_response;
    json j_request;

    vector<string> required_values {"address", "view_key"};

    if (!parse_request(body, required_values, j_request, j_response))
    {
        session_close(session, j_response.dump());
        return;
    }

    string xmr_address = j_request["address"];
    string view_key    = j_request["view_key"];


    // a placeholder for exciting or new account data
    XmrAccount acc;

    uint64_t acc_id {0};

    // first check if new account
    // select this account if its existing one
    if (!xmr_accounts->select(xmr_address, acc))
    {
        // account does not exist, so create new one
        // for this address

        uint64_t current_blockchain_height = get_current_blockchain_height();

        uint64_t current_blockchain_timestamp {0};

        // get last block so we have its timestamp when
        // createing the account
        block last_blk;

        if (CurrentBlockchainStatus::get_block(current_blockchain_height, last_blk))
        {
            current_blockchain_timestamp = last_blk.timestamp;
        }

        DateTime blk_timestamp_mysql_format
                = XmrTransaction::timestamp_to_DateTime(current_blockchain_timestamp);

        // we will save current blockchain height
        // in mysql, so that we know from what block
        // to start searching txs of this new acount
        // make it 1 block lower than current, just in case.
        // this variable will be our using to initialize
        // `scanned_block_height` in mysql Accounts table.
        if (acc_id = xmr_accounts->insert(xmr_address,
                                           make_hash(view_key),
                                           blk_timestamp_mysql_format,
                                           current_blockchain_height) == 0)
        {
            // if creating account failed
            j_response = json {{"status", "error"},
                               {"reason", "Account creation failed"}};

            session_close(session, j_response.dump());
            return;
        }
    } // if (!xmr_accounts->select(xmr_address, acc))


    // so by now new account has been created or it already exists
    // so we just login into it.

    if (login_and_start_search_thread(xmr_address, view_key, acc, j_response))
    {
       // if successfuly logged in and created search thread
        j_response["status"]      = "success";
        j_response["new_address"] = false;
    }
    else
    {
        // some error with loggin in or search thread start
        session_close(session, j_response.dump());
        return;

    } // else  if (login_and_start_search_thread(xmr_address, view_key, acc, j_response))


    session_close(session, j_response.dump());
}

void
YourMoneroRequests::get_address_txs(const shared_ptr< Session > session, const Bytes & body)
{
    json j_response;
    json j_request;

    vector<string> requested_values{"address", "view_key"};

    if (!parse_request(body, requested_values, j_request, j_response))
    {
        session_close(session, j_response.dump());
        return;
    }

    string xmr_address = j_request["address"];
    string view_key    = j_request["view_key"];

    // make hash of the submited viewkey. we only store
    // hash of viewkey in database, not acctual viewkey.
    string viewkey_hash = make_hash(view_key);

    // initialize json response
    j_response = json {
            {"total_received"         , 0},    // calculated in this function
            {"total_received_unlocked", 0},    // calculated in this function
            {"scanned_height"         , 0},    // not used. it is here to match mymonero
            {"scanned_block_height"   , 0},    // taken from Accounts table
            {"scanned_block_timestamp", 0},    // taken from Accounts table
            {"start_height"           , 0},    // blockchain hieght when acc was created
            {"blockchain_height"      , 0},    // current blockchain height
            {"transactions"           , json::array()}
    };

    // a placeholder for exciting or new account data
    xmreg::XmrAccount acc;

    // if not logged, i.e., no search thread exist, then start one.
    if (login_and_start_search_thread(xmr_address, view_key, acc, j_response))
    {
        // before fetching txs, check if provided view key
        // is correct. this is simply to ensure that
        // we cant fetch an account's txs using only address.
        // knowlage of the viewkey is also needed.



        uint64_t total_received {0};
        uint64_t total_received_unlocked {0};

        j_response["total_received"]          = total_received;
        j_response["start_height"]            = acc.start_height;
        j_response["scanned_block_height"]    = acc.scanned_block_height;
        j_response["scanned_block_timestamp"] = static_cast<uint64_t>(acc.scanned_block_timestamp);
        j_response["blockchain_height"]       = get_current_blockchain_height();

        vector<XmrTransaction> txs;

        if (xmr_accounts->select_txs_for_account_spendability_check(acc.id, txs))
        {
            json j_txs = json::array();

            for (XmrTransaction tx: txs)
            {
                json j_tx {
                        {"id"             , tx.blockchain_tx_id},
                        {"coinbase"       , bool {tx.coinbase}},
                        {"tx_pub_key"     , tx.tx_pub_key},
                        {"hash"           , tx.hash},
                        {"height"         , tx.height},
                        {"mixin"          , tx.mixin},
                        {"payment_id"     , tx.payment_id},
                        {"unlock_time"    , tx.unlock_time},                  
                        {"total_sent"     , 0},    // to be field when checking for spent_outputs below
                        {"total_received" , tx.total_received},
                        {"timestamp"      , static_cast<uint64_t>(tx.timestamp)},
                        {"mempool"        , false} // tx in database are never from mempool
                };

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
                              {"tx_pub_key" , tx.tx_pub_key},
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

    } // if (login_and_start_search_thread(xmr_address, view_key, acc, j_response))
    else
    {
        // some error with loggin in or search thread start
        session_close(session, j_response.dump());
        return;
    }

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
                //cout << "mempool j_tx[\"total_received\"]: "
                //     << j_tx["total_received"] << endl;

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
    json j_response;
    json j_request;

    vector<string> requested_values {"address" , "view_key"};

    if (!parse_request(body, requested_values, j_request, j_response))
    {
        session_close(session, j_response.dump());
        return;
    }

    const string& xmr_address = j_request["address"];
    const string& view_key    = j_request["view_key"];

    // make hash of the submited viewkey. we only store
    // hash of viewkey in database, not acctual viewkey.
    string viewkey_hash = make_hash(view_key);

    j_response = json {
            {"locked_funds"           , 0},    // locked xmr (e.g., younger than 10 blocks)
            {"total_received"         , 0},    // calculated in this function
            {"total_sent"             , 0},    // calculated in this function
            {"scanned_height"         , 0},    // not used. it is here to match mymonero
            {"scanned_block_height"   , 0},    // taken from Accounts table
            {"scanned_block_timestamp", 0},    // taken from Accounts table
            {"start_height"           , 0},    // not used, but available in Accounts table.
                                               // it is here to match mymonero
            {"blockchain_height"      , 0},    // current blockchain height
            {"spent_outputs"          , nullptr} // list of spent outputs that we think
                                               // user has spent. client side will
                                               // filter out false positives since
                                               // only client has spent key
    };

    // a placeholder for exciting or new account data
    xmreg::XmrAccount acc;

    // select this account if its existing one
    if (login_and_start_search_thread(xmr_address, view_key, acc, j_response))
    {

        uint64_t total_received {0};

        // ping the search thread that we still need it.
        // otherwise it will finish after some time.
        CurrentBlockchainStatus::ping_search_thread(xmr_address);

        uint64_t current_searched_blk_no {0};

        if (CurrentBlockchainStatus::get_searched_blk_no(xmr_address, current_searched_blk_no))
        {
            // if current_searched_blk_no is higher than what is in mysql, update it
            // in the search thread. This may occure when manually editing scanned_block_height
            // in Accounts table to import txs or rescan txs.
            // we use the minumum difference of 10 blocks, for this update to happen

            if (current_searched_blk_no > acc.scanned_block_height + 10)
            {
                CurrentBlockchainStatus::set_new_searched_blk_no(xmr_address, acc.scanned_block_height);
            }
        }

        j_response["total_received"]          = total_received;
        j_response["start_height"]            = acc.start_height;
        j_response["scanned_block_height"]    = acc.scanned_block_height;
        j_response["scanned_block_timestamp"] = static_cast<uint64_t>(acc.scanned_block_timestamp);
        j_response["blockchain_height"]       = get_current_blockchain_height();

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
                                    {"tx_pub_key" , tx.tx_pub_key},
                                    {"out_index"  , out.out_index},
                                    {"mixin"      , out.mixin},
                                });

                                total_sent += in.amount;
                            }
                        }

                        total_received += out.amount;

                    } //  for (XmrOutput &out: outs)

                } //  if (xmr_accounts->select_outputs_for_tx(tx.id, outs))

            } // for (XmrTransaction tx: txs)


            j_response["total_received"] = total_received;
            j_response["total_sent"]     = total_sent;

            j_response["spent_outputs"]  = j_spent_outputs;

        } // if (xmr_accounts->select_txs_for_account_spendability_check(acc.id, txs))

    } //  if (login_and_start_search_thread(xmr_address, view_key, acc, j_response))
    else
    {
        // some error with loggin in or search thread start
        session_close(session, j_response.dump());
        return;
    }

    string response_body = j_response.dump();

    auto response_headers = make_headers({{ "Content-Length", to_string(response_body.size())}});

    session->close( OK, response_body, response_headers);
}


void
YourMoneroRequests::get_unspent_outs(const shared_ptr< Session > session, const Bytes & body)
{
    json j_response;
    json j_request;

    vector<string> requested_values {"address" , "view_key", "mixin",
                                     "use_dust", "dust_threshold", "amount"};

    if (!parse_request(body, requested_values, j_request, j_response))
    {
        session_close(session, j_response.dump());
        return;
    }

    string xmr_address = j_request["address"];
    string view_key    = j_request["view_key"];

    uint64_t mixin     = j_request["mixin"];
    bool use_dust      = j_request["use_dust"];

    uint64_t dust_threshold = boost::lexical_cast<uint64_t>(j_request["dust_threshold"].get<string>());
    uint64_t amount         = boost::lexical_cast<uint64_t>(j_request["amount"].get<string>());


    // make hash of the submited viewkey. we only store
    // hash of viewkey in database, not acctual viewkey.
    string viewkey_hash = make_hash(view_key);

    j_response = json  {
            {"amount" , 0},            // total value of the outputs
            {"outputs", json::array()} // list of outputs
                                       // exclude those without require
                                       // no of confirmation
    };

    // a placeholder for exciting or new account data
    xmreg::XmrAccount acc;

    // select this account if its existing one
    if (login_and_start_search_thread(xmr_address, view_key, acc, j_response))
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

//                if (!bool {tx.coinbase})
//                {
//                    continue;
//                }

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

                        string rct = out.get_rct();

                        // coinbaser rct txs require speciall treatment
                        if (tx.coinbase && tx.is_rct)
                        {
                            uint64_t amount  = (tx.is_rct ? 0 : out.amount);

                            output_data_t od =
                                    CurrentBlockchainStatus::get_output_key(
                                            amount, global_amount_index);

                            string rtc_outpk  = pod_to_hex(od.commitment);
                            string rtc_mask   = pod_to_hex(rct::identity());
                            string rtc_amount(64, '0');

                            rct = rtc_outpk + rtc_mask + rtc_amount;
                        }

                        json j_out{
                                {"amount"          , out.amount},
                                {"public_key"      , out.out_pub_key},
                                {"index"           , out.out_index},
                                {"global_index"    , out.global_index},
                                {"rct"             , rct},
                                {"tx_id"           , out.tx_id},
                                {"tx_hash"         , tx.hash},
                                {"tx_prefix_hash"  , tx.prefix_hash},
                                {"tx_pub_key"      , tx.tx_pub_key},
                                {"timestamp"       , static_cast<uint64_t>(out.timestamp)},
                                {"height"          , tx.height},
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


        // need proper per_kb_fee estimate as
        // it is already using dynanamic fees. frontend
        // uses old fixed fees.

        uint64_t fee_estimated {DYNAMIC_FEE_PER_KB_BASE_FEE};

        if (CurrentBlockchainStatus::get_dynamic_per_kb_fee_estimate(fee_estimated))
        {
            j_response["per_kb_fee"] = fee_estimated;
        }


    } // if (login_and_start_search_thread(xmr_address, view_key, acc, j_response))
    else
    {
        // some error with loggin in or search thread start
        session_close(session, j_response.dump());
        return;

    }

    string response_body = j_response.dump();

    auto response_headers = make_headers({{ "Content-Length", to_string(response_body.size())}});

    session->close( OK, response_body, response_headers);
}

void
YourMoneroRequests::get_random_outs(const shared_ptr< Session > session, const Bytes & body)
{
    json j_request = body_to_json(body);

    uint64_t count = j_request["count"];

    json j_response  {
            {"amount_outs", json::array()}
    };

    vector<uint64_t> amounts;

    try
    {
        // populate amounts vector so that we can pass it directly to
        // daeamon to get random outputs for these amounts
        for (json amount: j_request["amounts"])
        {
            amounts.push_back(boost::lexical_cast<uint64_t>(amount.get<string>()));
        }
    }
    catch (boost::bad_lexical_cast& e)
    {
        cerr << "Bed lexical cast" << '\n';
        session_close(session, j_response.dump());
        return;
    }

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

                string rct = std::get<0>(rct_field)    // rct_pk
                             + std::get<1>(rct_field)  // rct_mask
                             + std::get<2>(rct_field); // rct_amount

                json out_details {
                        {"global_index", out.global_amount_index},
                        {"public_key"  , pod_to_hex(out.out_key)},
                        {"rct"         , rct}
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

    string raw_tx_blob = j_request["tx"];

    json j_response;

    string error_msg;

    if (!CurrentBlockchainStatus::commit_tx(
            raw_tx_blob, error_msg,
            CurrentBlockchainStatus::do_not_relay))
    {
        j_response["status"] = "error";
        j_response["error"]  = error_msg;
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

    string xmr_address   = j_request["address"];

    // a placeholder for existing or new payment data
    xmreg::XmrPayment xmr_payment;

    json j_response;

    j_response["request_fulfilled"] = false;
    j_response["status"] = "error";
    j_response["error"]  = "Some error occured";

    // select this payment if its existing one
    if (xmr_accounts->select_payment_by_address(xmr_address, xmr_payment))
    {
        // payment record exists, so now we need to check if
        // actually payment has been done, and updated
        // mysql record accordingly.

        bool request_fulfilled = bool {xmr_payment.request_fulfilled};

        string integrated_address =
                CurrentBlockchainStatus::get_account_integrated_address_as_str(
                        xmr_payment.payment_id);

        j_response["payment_id"]        = xmr_payment.payment_id;
        j_response["import_fee"]        = xmr_payment.import_fee;
        j_response["new_request"]       = false;
        j_response["request_fulfilled"] = request_fulfilled;
        j_response["payment_address"]   = integrated_address;
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
                            j_response["error"] = "Updating searched_blk_no failed!";
                        }
                    }
                }
                else
                {
                    cerr << "Updating accounts due to made payment mysql failed! " << endl;
                    j_response["error"] = "Updating accounts due to made payment mysql failed!";
                }
            }
            else
            {
                cerr << "Updating payment mysql failed! " << endl;
                j_response["error"] = "Updating payment mysql failed!";
            }
        }

        if (request_fulfilled)
        {
            j_response["request_fulfilled"] = request_fulfilled;
            j_response["status"]            = "Payment received. Thank you.";
            j_response["error"]             = "";
        }
    }
    else
    {
        // payment request is new, so create its entry in
        // Payments table

        uint64_t payment_table_id {0};

        crypto::hash8 random_payment_id8 = crypto::rand<crypto::hash8>();

        string integrated_address =
                CurrentBlockchainStatus::get_account_integrated_address_as_str(
                        random_payment_id8);

        xmr_payment.address           = xmr_address;
        xmr_payment.payment_id        = pod_to_hex(random_payment_id8);
        xmr_payment.import_fee        = CurrentBlockchainStatus::import_fee; // xmr
        xmr_payment.request_fulfilled = false;
        xmr_payment.tx_hash           = ""; // no tx_hash yet with the payment
        xmr_payment.payment_address   = integrated_address;

        if ((payment_table_id = xmr_accounts->insert_payment(xmr_payment)) != 0)
        {
            // payment entry created

            j_response["payment_id"]        = xmr_payment.payment_id;
            j_response["import_fee"]        = xmr_payment.import_fee;
            j_response["new_request"]       = true;
            j_response["request_fulfilled"] = bool {xmr_payment.request_fulfilled};
            j_response["payment_address"]   = xmr_payment.payment_address;
            j_response["status"]            = "Payment not yet received";
            j_response["error"]             = "";
        }
    }

    string response_body = j_response.dump();

    auto response_headers = make_headers({{ "Content-Length", to_string(response_body.size())}});

    session->close( OK, response_body, response_headers);
}



void
YourMoneroRequests::import_recent_wallet_request(const shared_ptr< Session > session, const Bytes & body)
{
    json j_response;
    json j_request;

    bool request_fulfilled {false};

    j_response["request_fulfilled"] = false;

    vector<string> requested_values {"address" , "view_key", "no_blocks_to_import"};

    if (!parse_request(body, requested_values, j_request, j_response))
    {
        j_response["Error"] = "Cant parse json body";
        session_close(session, j_response.dump());
        return;
    }

    string xmr_address           = j_request["address"];
    string view_key              = j_request["view_key"];

    uint64_t no_blocks_to_import {1000};

    try
    {
        no_blocks_to_import = boost::lexical_cast<uint64_t>(j_request["no_blocks_to_import"].get<string>());
    }
    catch (boost::bad_lexical_cast& e)
    {
        string msg = "Cant parse " + j_request["no_blocks_to_import"].get<string>() + " into number";

        cerr << msg << '\n';

        j_response["Error"] = msg;
        session_close(session, j_response.dump());
        return;
    }

    // make sure that we dont import more that the maximum alowed no of blocks
    no_blocks_to_import = std::min(no_blocks_to_import,
                                   CurrentBlockchainStatus::max_number_of_blocks_to_import);

    XmrAccount acc;

    if (xmr_accounts->select(xmr_address, acc))
    {
        XmrAccount updated_acc = acc;

        // make sure scanned_block_height is larger than  no_blocks_to_import so we dont
        // end up with overflowing uint64_t.

        if (updated_acc.scanned_block_height > no_blocks_to_import)
        {
            // repetead calls to import_recent_wallet_request will be moving the scanning backward.
            // not sure yet if any protection is needed to make sure that a user does not
            // go back too much back by importing his/hers wallet multiple times in a row.
            updated_acc.scanned_block_height = updated_acc.scanned_block_height - no_blocks_to_import;

            if (xmr_accounts->update(acc, updated_acc))
            {
                // change search blk number in the search thread
                if (!CurrentBlockchainStatus::set_new_searched_blk_no(xmr_address,
                                                                      updated_acc.scanned_block_height))
                {
                    cerr << "Updating searched_blk_no failed!" << endl;
                    j_response["Error"]  = "Updating searched_blk_no failed!";
                }
                else
                {
                    // if success, makre that request was successful;
                    request_fulfilled = true;
                }
            }

        }  // if (updated_acc.scanned_block_height > no_blocks_to_import)
    }
    else
    {
        cerr << "Updating account with new scanned_block_height failed! " << endl;
        j_response["status"] = "Updating account with new scanned_block_height failed!";
    }

    if (request_fulfilled)
    {
        j_response["request_fulfilled"] = request_fulfilled;
        j_response["status"]            = "Updating account with for importing recent txs successeful.";
    }

    string response_body = j_response.dump();

    auto response_headers = make_headers({{ "Content-Length", to_string(response_body.size())}});

    session->close( OK, response_body, response_headers);
}


void
YourMoneroRequests::get_version(const shared_ptr< Session > session, const Bytes & body)
{

    json j_response {
        {"last_git_commit_hash", string {GIT_COMMIT_HASH}},
        {"last_git_commit_date", string {GIT_COMMIT_DATETIME}},
        {"git_branch_name"     , string {GIT_BRANCH_NAME}},
        {"monero_version_full" , string {MONERO_VERSION_FULL}},
        {"blockchain_height"   , get_current_blockchain_height()}
    };

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

    session->fetch(content_length, [](const shared_ptr< Session > session, const Bytes & body)
    {
        session->close( OK, string{}, make_headers());
    });
}


multimap<string, string>
YourMoneroRequests::make_headers(const multimap<string, string>& extra_headers)
{
    multimap<string, string> headers {
            {"Access-Control-Allow-Origin"     , "*"},
            {"Access-Control-Allow-Headers"    , "Content-Type"},
            {"Content-Type"                    , "application/json"}
    };

    headers.insert(extra_headers.begin(), extra_headers.end());

    return headers;
};

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

bool
YourMoneroRequests::login_and_start_search_thread(
                        const string& xmr_address,
                        const string& view_key,
                        XmrAccount& acc,
                        json& j_response)
{

    // select this account if its existing one
    if (xmr_accounts->select(xmr_address, acc))
    {
        // we got accunt from the database. we double check
        // if hash of provided viewkey by the frontend, matches
        // what we have in database.

        // make hash of the submited viewkey. we only store
        // hash of viewkey in database, not acctual viewkey.
        string viewkey_hash = make_hash(view_key);

        if (viewkey_hash == acc.viewkey_hash)
        {
            // if match, than save the viewkey in account object
            // and proceed to checking if a search thread exisits
            // for this account. if not, then create new thread

            acc.viewkey = view_key;

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

                j_response["status"]      = "success";
                j_response["new_address"] = false;

                // thread has been started or already exists.
                // everything seems fine.

                return true;
            }
            else
            {
                j_response = json {{"status", "error"},
                                   {"reason", "Failed created search thread for this account"}};
            }

        }
        else
        {
            j_response = json {{"status", "error"},
                               {"reason", "Viewkey provided is incorrect"}};
        }
    }

    return false;
}



void
YourMoneroRequests::session_close(const shared_ptr< Session > session, string response_body)
{
    auto response_headers = make_headers({{"Content-Length", to_string(response_body.size())}});
    session->close(OK, response_body, response_headers);
}


bool
YourMoneroRequests::parse_request(
        const Bytes& body,
        vector<string>& values_map,
        json& j_request,
        json& j_response)
{

    try
    {
        j_request = body_to_json(body);

        // parsing was successful
        // now check if all required values are there.

        for (const auto& v: values_map)
        {

            if (j_request.count(v) == 0)
            {
                cerr << v + " value not provided" << endl;

                j_response["status"] = "error";
                j_response["reason"] = v + " value not provided";

                return false;
            }

        }

        return true;
    }
    catch (std::exception& e)
    {
        cerr << "YourMoneroRequests::parse_request: " << e.what() << endl;

        j_response["status"] = "error";
        j_response["reason"] = "reqest json parsing failed";

        return false;
    }
}

}



