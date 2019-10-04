//
// Created by mwo on 8/01/17.
//

#define MYSQLPP_SSQLS_NO_STATICS 1


#include "OpenMoneroRequests.h"
#include "src/UniversalIdentifier.hpp"

#include "db/ssqlses.h"

#include "version.h"
#include "../gen/omversion.h"

namespace xmreg
{

handel_::handel_(const fetch_func_t& callback):
        request_callback {callback}
{}

void
handel_::operator()(const shared_ptr< Session > session)
{
    const auto request = session->get_request( );
    size_t content_length = request->get_header("Content-Length", 0);
    session->fetch(content_length, this->request_callback);
}



OpenMoneroRequests::OpenMoneroRequests(
        shared_ptr<MySqlAccounts> _acc, 
        shared_ptr<CurrentBlockchainStatus> _current_bc_status):
    xmr_accounts {_acc}, current_bc_status {_current_bc_status}
{

}


void
OpenMoneroRequests::login(const shared_ptr<Session> session, const Bytes & body)
{
    json j_response;
    json j_request;

    vector<string> required_values {"address", "view_key"};

    if (!parse_request(body, required_values, j_request, j_response))
    {
        session_close(session, j_response);
        return;
    }

    string xmr_address;
    string view_key;

    // this is true for both newly created accounts
    // and existing/imported ones
    bool create_accountt {true};

    // client sends generated_localy as true
    // for new accounts, and false for 
    // adding existing accounts (i.e., importing wallet)
    bool generated_locally {false};

    try
    {
        xmr_address       = j_request["address"];
        view_key          = j_request["view_key"];
        create_accountt   = j_request["create_account"];
        if (j_request.count("generated_locally"))
            generated_locally = j_request["generated_locally"];
    }
    catch (json::exception const& e)
    {
        OMERROR << "json exception: " << e.what();
        session_close(session, j_response);
        return;
    }

    if (create_accountt == false)
    {
        j_response = json {{"status", "error"},
                           {"reason", "Not making an account"}};

        session_close(session, j_response);
        return;
    }


    // a placeholder for exciting or new account data id
    uint64_t acc_id {0};

    // marks if this is new account creation or not
    bool new_account_created {false};

    auto acc = select_account(xmr_address, view_key, false);

    // first check if new account
    // select this account if its existing one
    if (!acc)
    {
        // account does not exist, so create new one
        // for this address
        if (!(acc = create_account(xmr_address, view_key, 
                                   generated_locally)))
        {
            // if creating account failed
            j_response = json {{"status", "error"},
                               {"reason", "Account creation failed"}};

            session_close(session, j_response);
            return;
        }
    
        // set this flag to indicate that we have just created a
        // new account in mysql. this information is sent to front-end
        // as it can disply some greeting window to new users upon
        // their first entry 
        new_account_created = true;

    } // if (!acc)
        
    
    j_response["generated_locally"] = bool {acc->generated_locally};

    j_response["start_height"] = acc->start_height;


    // so by now new account has been created or it already exists
    // so we just login into it.
    
    if (login_and_start_search_thread(xmr_address, view_key, *acc, j_response))
    {
       // if successfuly logged in and created search thread
       j_response["status"]      = "success";

       // we overwrite what ever was sent in login_and_start_search_thread
       // for the j_response["new_address"].
       j_response["new_address"] = new_account_created;
    }
    else
    {
        // some error with loggin in or search thread start
        OMERROR << xmr_address.substr(0,6) << ": " 
                << "login_and_start_search_thread failed. "
                << j_response.dump();

        session_close(session, j_response);
        return;

    } // else  if (login_and_start_search_thread(xmr_address,


    session_close(session, j_response);
}

void
OpenMoneroRequests::ping(const shared_ptr<Session> session, const Bytes & body)
{
    json j_response;
    json j_request;

    vector<string> required_values {"address", "view_key"};

    if (!parse_request(body, required_values, j_request, j_response))
    {
        session_close(session, j_response);
        return;
    }

    string xmr_address;
    string view_key;

    try
    {
        xmr_address = j_request["address"];
        view_key    = j_request["view_key"];
    }
    catch (json::exception const& e)
    {
        OMERROR << "json exception: " << e.what();
        session_close(session, j_response, UNPROCESSABLE_ENTITY);
        return;
    }

    if (!current_bc_status->search_thread_exist(xmr_address, view_key))
    {
        OMERROR << xmr_address.substr(0,6) + ": search thread does not exist";
        session_close(session, j_response, UNPROCESSABLE_ENTITY);
        return;
    }

    // ping the search thread that we still need it.
    // otherwise it will finish after some time.
    if (!current_bc_status->ping_search_thread(xmr_address))
    {
        j_response = json {{"status", "error"},
                           {"reason", "Pinging search thread failed."}};

        // some error with loggin in or search thread start
        session_close(session, j_response);
        return;

    }

    OMINFO << xmr_address.substr(0,6) + ": search thread ping successful";
    
    j_response["status"]  = "success";
    
    session_close(session, j_response);
}

void
OpenMoneroRequests::get_address_txs(
        const shared_ptr< Session > session, const Bytes & body)
{
    json j_response;
    json j_request;

    vector<string> requested_values {"address", "view_key"};

    if (!parse_request(body, requested_values, j_request, j_response))
    {
        session_close(session, j_response);
        return;
    }

    string xmr_address;
    string view_key;

    try
    {
        xmr_address = j_request["address"];
        view_key    = j_request["view_key"];
    }
    catch (json::exception const& e)
    {
        OMERROR << "json exception: " << e.what();
        session_close(session, j_response, UNPROCESSABLE_ENTITY);
        return;
    }

    // make hash of the submited viewkey. we only store
    // hash of viewkey in database, not acctual viewkey.
    string viewkey_hash = make_hash(view_key);

    // initialize json response
    j_response = json {
            {"total_received"         , 0},    // calculated in this function
            {"total_received_unlocked", 0},    // calculated in this function
            {"scanned_height"         , 0},    // not used. just to match mymonero
            {"scanned_block_height"   , 0},    // taken from Accounts table
            {"scanned_block_timestamp", 0},    // taken from Accounts table
            {"start_height"           , 0},    // blockchain height whencreated
            {"blockchain_height"      , 0},    // current blockchain height
            {"transactions"           , json::array()}
    };

    // a placeholder for exciting or new account data
    xmreg::XmrAccount acc;

    // for this to continue, search thread must have already been
    // created and still exisits.
    if (!login_and_start_search_thread(xmr_address, view_key, acc, j_response))
    {
        j_response = json {{"status", "error"},
                           {"reason", "Search thread does not exist."}};

        // some error with loggin in or search thread start
        session_close(session, j_response);
        return;
    }

    // before fetching txs, check if provided view key
    // is correct. this is simply to ensure that
    // we cant fetch an account's txs using only address.
    // knowlage of the viewkey is also needed.

    uint64_t total_received {0};
    uint64_t total_received_unlocked {0};

    j_response["total_received"]          = total_received;
    j_response["start_height"]            = acc.start_height;
    j_response["scanned_block_height"]    = acc.scanned_block_height;
    j_response["scanned_block_timestamp"] = static_cast<uint64_t>(
                acc.scanned_block_timestamp);
    j_response["blockchain_height"]  = get_current_blockchain_height();

    vector<XmrTransaction> txs;

    xmr_accounts->select(acc.id.data, txs);

    if (xmr_accounts->select_txs_for_account_spendability_check(
                acc.id.data, txs))
    {
        json j_txs = json::array();

        for (XmrTransaction const& tx: txs)
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
                    {"total_sent"     , 0}, // to be field when checking for spent_outputs below
                    {"total_received" , std::to_string(tx.total_received)},
                    {"timestamp"      , static_cast<uint64_t>(tx.timestamp)*1000},
                    {"mempool"        , false} // tx in database are never from mempool
            };

            vector<XmrInput> inputs;

            if (xmr_accounts->select_for_tx(tx.id.data, inputs))
            {
                json j_spent_outputs = json::array();

                uint64_t total_spent {0};

                for (XmrInput input: inputs)
                {
                    XmrOutput out;

                    if (xmr_accounts->select_by_primary_id(
                                input.output_id, out))
                    {
                        total_spent += input.amount;

                        j_spent_outputs.push_back({
                          {"amount"     , std::to_string(input.amount)},
                          {"key_image"  , input.key_image},
                          {"tx_pub_key" , out.tx_pub_key},
                          {"out_index"  , out.out_index},
                          {"mixin"      , out.mixin}});
                    }
                }

                j_tx["total_sent"] = std::to_string(total_spent);

                j_tx["spent_outputs"] = j_spent_outputs;

            } // if (xmr_accounts->select_inputs_for_tx(tx.id, inputs))

            total_received += tx.total_received;

            if (bool {tx.spendable})
            {
                total_received_unlocked += tx.total_received;
            }

            j_txs.push_back(j_tx);

        } // for (XmrTransaction tx: txs)

        j_response["total_received"]          = std::to_string(total_received);
        j_response["total_received_unlocked"] = std::to_string(total_received_unlocked);

        j_response["transactions"] = j_txs;

    } // if (xmr_accounts->select_txs_for_ac

    // append txs found in mempool to the json returned

    json j_mempool_tx;

    if (current_bc_status->find_txs_in_mempool(
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

                total_received_mempool += boost::lexical_cast<uint64_t>(
                            j_tx["total_received"].get<string>());
                total_sent_mempool     += boost::lexical_cast<uint64_t>(
                            j_tx["total_sent"].get<string>());

                j_response["transactions"].push_back(j_tx);
            }

            // we account for mempool txs when providing final
            // unlocked and locked balances.

            j_response["total_received"]
                    = std::to_string(
                        boost::lexical_cast<uint64_t>(
                            j_response["total_received"].get<string>())
                                           + total_received_mempool - total_sent_mempool);

            j_response["total_received_unlocked"]
                    = std::to_string(
                        boost::lexical_cast<uint64_t>(
                            j_response["total_received_unlocked"].get<string>())
                                          + total_received_mempool - total_sent_mempool);

        } //if(!j_mempool_tx.empty())

    } // current_bc_status->find_txs_in_mempool

    string response_body = j_response.dump();

    auto response_headers = make_headers(
            {{ "Content-Length", to_string(response_body.size()) }});

    session->close(OK, response_body, response_headers);
}

void
OpenMoneroRequests::get_address_info(
        const shared_ptr< Session > session, const Bytes & body)
{
    json j_response;
    json j_request;

    vector<string> requested_values {"address" , "view_key"};

    if (!parse_request(body, requested_values, j_request, j_response))
    {
        session_close(session, j_response);
        return;
    }

    string xmr_address;
    string view_key;

    try
    {
        xmr_address = j_request["address"];
        view_key    = j_request["view_key"];
    }
    catch (json::exception const& e)
    {
        OMERROR << "json exception: " << e.what();
        session_close(session, j_response);
        return;
    }

    // make hash of the submited viewkey. we only store
    // hash of viewkey in database, not acctual viewkey.
    string viewkey_hash = make_hash(view_key);

    j_response = json {
            {"locked_funds"           , "0"},    // locked xmr (e.g., younger than 10 blocks)
            {"total_received"         , "0"},    // calculated in this function
            {"total_sent"             , "0"},    // calculated in this function
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

    // for this to continue, search thread must have already been
    // created and still exisits.
    if (login_and_start_search_thread(xmr_address, view_key, acc, j_response))
    {
        uint64_t total_received {0};

        // ping the search thread that we still need it.
        // otherwise it will finish after some time.
        current_bc_status->ping_search_thread(xmr_address);

        uint64_t current_searched_blk_no {0};

        if (current_bc_status->get_searched_blk_no(
                    xmr_address, current_searched_blk_no))
        {
            // if current_searched_blk_no is higher than what is in mysql, update it
            // in the search thread. This may occure when manually editing scanned_block_height
            // in Accounts table to import txs or rescan txs.
            // we use the minumum difference of 10 blocks, for this update to happen

            if (current_searched_blk_no > acc.scanned_block_height + 10)
            {
                current_bc_status->set_new_searched_blk_no(
                            xmr_address, acc.scanned_block_height);
            }
        }

        j_response["total_received"]          = std::to_string(total_received);
        j_response["start_height"]            = acc.start_height;
        j_response["scanned_block_height"]    = acc.scanned_block_height;
        j_response["scanned_block_timestamp"]
                = static_cast<uint64_t>(acc.scanned_block_timestamp);
        j_response["blockchain_height"]  = get_current_blockchain_height();

        uint64_t total_sent {0};

        vector<XmrTransaction> txs;

        // get all txs of for the account
        xmr_accounts->select(acc.id.data, txs);

        // now, filter out or updated transactions from txs vector that no
        // longer exisit in the recent blocks. Update is done to check for their
        // spendability status.
        if (xmr_accounts->select_txs_for_account_spendability_check(
                    acc.id.data, txs))
        {
            json j_spent_outputs = json::array();

            for (XmrTransaction tx: txs)
            {
                vector<XmrOutput> outs;

                if (xmr_accounts->select_for_tx(tx.id.data, outs))
                {
                    for (XmrOutput &out: outs)
                    {
                        // check if the output, has been spend
                        vector<XmrInput> ins;

                        if (xmr_accounts->select_inputs_for_out(
                                    out.id.data, ins))
                        {
                            for (XmrInput& in: ins)
                            {
                                j_spent_outputs.push_back({
                                    {"amount"     , std::to_string(in.amount)},
                                    {"key_image"  , in.key_image},
                                    {"tx_pub_key" , out.tx_pub_key},
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


            j_response["total_received"] = std::to_string(total_received);
            j_response["total_sent"]     = std::to_string(total_sent);

            j_response["spent_outputs"]  = j_spent_outputs;

        } // if (xmr_accounts->select_txs_for_account_spendability_check(acc.id, txs))

    } // if (current_bc_status->search_thread_exist(xmr_address))
    else
    {
        j_response = json {{"status", "error"},
                           {"reason", "Search thread does not exist."}};

        // some error with loggin in or search thread start
        session_close(session, j_response);
        return;
    }

    string response_body = j_response.dump();

    auto response_headers = make_headers({{ "Content-Length",
                                            to_string(response_body.size())}});

    session->close( OK, response_body, response_headers);
}


void
OpenMoneroRequests::get_unspent_outs(
        const shared_ptr< Session > session,
        const Bytes & body)
{
    json j_response;
    json j_request;

    vector<string> requested_values {"address" , "view_key", "mixin",
                                     "use_dust", "dust_threshold", "amount"};

    if (!parse_request(body, requested_values, j_request, j_response))
    {
        session_close(session, j_response);
        return;
    }

    string xmr_address;
    string view_key;
    uint64_t mixin {4};
    bool use_dust {false};
    uint64_t dust_threshold {1000000000};
    uint64_t amount {0};

    try
    {
        xmr_address = j_request["address"];
        view_key    = j_request["view_key"];

        mixin       = j_request["mixin"];
        use_dust    = j_request["use_dust"];

        dust_threshold = boost::lexical_cast<uint64_t>(
                    j_request["dust_threshold"].get<string>());
        amount  = boost::lexical_cast<uint64_t>(
                    j_request["amount"].get<string>());
    }
    catch (json::exception const& e)
    {
        OMERROR << "json exception: " << e.what();
        session_close(session, j_response);
        return;
    }
    catch (boost::bad_lexical_cast const& e)
    {
        OMERROR << "Bed lexical cast" << e.what() << '\n';
        session_close(session, j_response);
        return;
    }

    // make hash of the submited viewkey. we only store
    // hash of viewkey in database, not acctual viewkey.
    string viewkey_hash = make_hash(view_key);

    j_response = json  {
            {"amount" , "0"},          // total value of the outputs
            {"fork_version", 
                current_bc_status->get_hard_fork_version()},
            {"outputs", json::array()} // list of outputs
                                       // exclude those without require
                                       // no of confirmation
    };

    // a placeholder for exciting or new account data
    xmreg::XmrAccount acc;

    // select this account if its existing one

    // for this to continue, search thread must have already been
    // created and still exisits.
    if (current_bc_status->search_thread_exist(xmr_address))
    {
        // populate acc and check view_key
        if (!login_and_start_search_thread(xmr_address, view_key, acc, j_response))
        {
            // some error with loggin in or search thread start
            session_close(session, j_response);
            return;
        }

        uint64_t total_outputs_amount {0};

//        uint64_t current_blockchain_height
//                = current_bc_status->get_current_blockchain_height();

        vector<XmrTransaction> txs;

        // retrieve txs from mysql associated with the given address
        if (xmr_accounts->select(acc.id.data, txs))
        {
            // we found some txs.

            json& j_outputs = j_response["outputs"];

            for (XmrTransaction& tx: txs)
            {
                // we skip over locked outputs
                // as they cant be spent anyway.
                // thus no reason to return them to the frontend
                // for constructing a tx.

                if (!current_bc_status->is_tx_unlocked(
                            tx.unlock_time, tx.height))
                {
                    continue;
                }


                vector<XmrOutput> outs;

                if (!xmr_accounts->select_for_tx(tx.id.data, outs))
                {
                    continue;
                }

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

                    // default case. it will cover 
                    // rct types 1 (Full) and 2 (Simple)
                    // rct types explained here: 
                    // https://monero.stackexchange.com/questions/3348/what-are-3-types-of-ring-ct-transactions
                    string rct = out.get_rct();

                    // based on 
                    // https://github.com/mymonero/mymonero-app-js/issues/277#issuecomment-469395825

                    if (!tx.is_rct)
                    {
                        // point 1: null/undefined/empty: 
                        // non-RingCT output (i.e, from version 1 tx)
                        // covers all pre-ringct outputs
                        rct = "";
                    }
                    else    
                    {
                        // for RingCT:
                                                     
                       if (tx.rct_type == 0)
                       {
                       // coinbase rct txs require speciall treatment
                       // point 2: string "coinbase" (length 8): 
                       // RingCT coinbase output
                       
                        rct = "coinbase";
                       }
                       else if (tx.rct_type == 3)
                       {                               
                           // point 3: string length 192: non-coinbase RingCT 
                           // version 1 output with 256-bit amount and mask
                           // rct type 3 is Booletproof
                           
                           rct = out.rct_outpk + out.rct_mask + out.rct_amount;
                       }
                       else if (tx.rct_type == 4)
                       {
                           // point 4 string length 80: 
                           // non-coinbase RingCT version 2 
                           // output 64 bit amount
                           // rct type 4 is Booletproof2
                           
                           rct = out.rct_outpk + out.rct_amount.substr(0,16);
                       }
                    }

                    //cout << "tx hash: " << tx.hash  << ", rtc: " << rct 
                    //     << ", rtc size: " << rct.size()  
                    //     << ", decrypted mask: " << out.rct_mask
                    //     << endl;

                    json j_out{
                            {"amount"          , std::to_string(out.amount)},
                            {"public_key"      , out.out_pub_key},
                            {"index"           , out.out_index},
                            {"global_index"    , out.global_index},
                            {"rct"             , rct},
                            {"tx_id"           , out.tx_id},
                            {"tx_hash"         , tx.hash},
                            {"tx_prefix_hash"  , tx.prefix_hash},
                            {"tx_pub_key"      , tx.tx_pub_key},
                            {"timestamp"       , static_cast<uint64_t>(
                                        out.timestamp*1e3)},
                            {"height"          , tx.height},
                            {"spend_key_images", json::array()}
                    };

                    vector<XmrInput> ins;

                    if (xmr_accounts->select_inputs_for_out(
                                out.id.data, ins))
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

            } // for (XmrTransaction& tx: txs)

        } //  if (xmr_accounts->select_txs(acc.id, txs))

        j_response["amount"] = std::to_string(total_outputs_amount);


        // need proper per_kb_fee estimate as
        // it is already using dynanamic fees. frontend
        // uses old fixed fees.

        j_response["per_byte_fee"] = current_bc_status
                                            ->get_dynamic_base_fee_estimate();

    } // if (current_bc_status->search_thread_exist(xmr_address))
    else
    {
        j_response = json {{"status", "error"},
                           {"reason", "Search thread does not exist."}};

        // some error with loggin in or search thread start
        session_close(session, j_response);
        return;

    }

    string response_body = j_response.dump();

    auto response_headers = make_headers({{ "Content-Length",
                                            to_string(response_body.size())}});

    session->close( OK, response_body, response_headers);
}

void
OpenMoneroRequests::get_random_outs(
        const shared_ptr< Session > session, const Bytes & body)
{
    json j_request;
    json j_response;

    vector<string> requested_values;

    if (!parse_request(body, requested_values, j_request, j_response))
    {
        session_close(session, j_response);
        return;
    }

    uint64_t count;

    try
    {
        count = j_request["count"];
    }
    catch (json::exception const& e)
    {
        OMERROR << "json exception: " << e.what();
        session_close(session, j_response);
        return;
    };

    if (count > 41)
    {
        OMERROR << "Request ring size too big" << '\n';
        j_response["status"] = "error";
        j_response["error"]  = "Request ring size too large";
        session_close(session, j_response);
    }

    vector<uint64_t> amounts;

    try
    {
        // populate amounts vector so that we can pass it directly to
        // daeamon to get random outputs for these amounts
        for (json amount: j_request["amounts"])
        {
            amounts.push_back(boost::lexical_cast<uint64_t>(
                                  amount.get<string>()));

            //            amounts.push_back(0);
        }
    }
    catch (boost::bad_lexical_cast& e)
    {
        OMERROR << "Bed lexical cast";
        session_close(session, j_response);
        return;
    }

    vector<RandomOutputs::outs_for_amount> found_outputs;

    if (current_bc_status->get_random_outputs(amounts, count, found_outputs))
    {
        json& j_amount_outs = j_response["amount_outs"];

        for (const auto& outs: found_outputs)
        {
            json j_outs {{"amount", std::to_string(outs.amount)},
                         {"outputs", json::array()}};


            json& j_outputs = j_outs["outputs"];

            for (auto  const& out: outs.outs)
            {
                tuple<string, string, string>
                        rct_field = current_bc_status
                            ->construct_output_rct_field(
                                    out.global_amount_index, outs.amount);

                string rct = std::get<0>(rct_field)    // rct_pk
                             + std::get<1>(rct_field)  // rct_mask
                             + std::get<2>(rct_field); // rct_amount

                
    

                json out_details {
                        {"global_index", out.global_amount_index},
                        {"public_key"  , pod_to_hex(out.out_key)},
                        {"rct"         , rct}
                };

                j_outputs.push_back(out_details);

            } // for (const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AM

            j_amount_outs.push_back(j_outs);

        } // for (const COMMAND_RPC_GET_RANDOM_OUTPUTS_

    } // if (current_bc_status->get_random_outputs(amounts,
    else
    {
        j_response["status"] = "error";
        j_response["error"]  = "Error getting random "
                               "outputs from monero deamon";
    }

    string response_body = j_response.dump();

    auto response_headers = make_headers({{ "Content-Length",
                                            to_string(response_body.size())}});

    session->close( OK, response_body, response_headers);
}


void
OpenMoneroRequests::submit_raw_tx(
        const shared_ptr< Session > session, const Bytes & body)
{
    json j_request = body_to_json(body);

    string raw_tx_blob = j_request["tx"];

    json j_response;

    string error_msg;

    // before we submit the tx into the deamon, we are going to do a few checks.
    // first, we parse the hexbuffer submmited by the frontend into
    // binary buffer block
    // second, we are going to check if we can construct
    // valid transaction object
    // fromt that binary buffer.
    // third, we are going to check if any key image in this tx is
    // in any of the txs
    // in the mempool. This allows us to make a clear comment that the tx
    // uses outputs just spend. This happens we a users submits few txs,
    // one after another
    // before previous txs get included in a block and are still
    // present in the mempool.

    std::string tx_blob;

    if(!epee::string_tools::parse_hexstr_to_binbuff(raw_tx_blob, tx_blob))
    {
        error_msg  = "Tx faild parse_hexstr_to_binbuff";

        OMERROR << error_msg;

        session_close(session, j_response,
                      UNPROCESSABLE_ENTITY,
                      error_msg);
        return;
    }

    transaction tx_to_be_submitted;

    if (!parse_and_validate_tx_from_blob(tx_blob, tx_to_be_submitted))
    {
        error_msg = "Tx faild parse_and_validate_tx_from_blob";

        OMERROR << error_msg;

        session_close(session, j_response,
                      UNPROCESSABLE_ENTITY, 
                      error_msg);
        return;
    }

    if (current_bc_status->find_key_images_in_mempool(tx_to_be_submitted))
    {
        error_msg =  "Tx uses your outputs that area already "
                               "in the mempool. "
                               "Please wait till your previous tx(s) "
                               "get mined";

        OMERROR << error_msg;

        session_close(session, j_response,
                      UNPROCESSABLE_ENTITY,
                      error_msg);
        return;
    }

    if (!current_bc_status->commit_tx(
            raw_tx_blob, error_msg,
            current_bc_status->get_bc_setup().do_not_relay))
    {
        OMERROR << error_msg;

        session_close(session, j_response,
                      UNPROCESSABLE_ENTITY,
                      error_msg);
        return;
    }


    string response_body = j_response.dump();

    auto response_headers = make_headers({{ "Content-Length",
                                            to_string(response_body.size())}});

    session->close( OK, response_body, response_headers);
}

//@todo current import_wallet_request end point
// still requires some work. The reason is that 
// at this moment it is not clear how it is
// handled in mymonero-app-js
void
OpenMoneroRequests::import_wallet_request(
        const shared_ptr< Session > session, const Bytes & body)
{

    json j_response;
    json j_request;

    vector<string> requested_values {"address" , "view_key"};

    if (!parse_request(body, requested_values,
                       j_request, j_response))
    {
        session_close(session, j_response, UNPROCESSABLE_ENTITY,
                      "Cant parse json body!");
        return;
    }

    string xmr_address;
    string view_key;

    try
    {
        xmr_address = j_request["address"];
        view_key    = j_request["view_key"];
    }
    catch (json::exception const& e)
    {
        OMWARN << "json exception: " << e.what();
        session_close(session, j_response, UNPROCESSABLE_ENTITY,
                      e.what());
        return;
    }


    j_response["request_fulfilled"] = false;
    j_response["import_fee"]        = std::to_string(
                                            current_bc_status->get_bc_setup()
                                                .import_fee);
    j_response["status"] = "error";
    j_response["error"]  = "Some error occured";

    // get account from mysql db if exists
    auto xmr_account = select_account(xmr_address, view_key);

    if (!xmr_account)
    {
        // if creation failed, just close the session
        session_close(session, j_response, UNPROCESSABLE_ENTITY,
                      "The account login or creation failed!");
        return;
    }

    auto import_fee = current_bc_status->get_bc_setup().import_fee;

    // if import is free than just upadte mysql and set new 
    // tx search block 
    if (import_fee == 0)
    {
        
        XmrAccount updated_acc = *xmr_account;

        updated_acc.scanned_block_height = 0;
        updated_acc.start_height = 0;

        // set scanned_block_height	to 0 to begin
        // scanning entire blockchain



        // @todo we will have race condition here
        // as we updated mysql here, but at the same time 
        // txsearch tread does the same thing
        // and just few lines blow we update_acc yet again

        if (!xmr_accounts->update(*xmr_account, updated_acc))
        {
            OMERROR << xmr_address.substr(0,6) +
                        "Updating scanned_block_height failed!\n";

            session_close(session, j_response, UNPROCESSABLE_ENTITY,
                          "Updating scanned_block_height failed!");
            return;
        }
        
        // change search blk number in the search thread
        if (!current_bc_status->set_new_searched_blk_no(xmr_address, 0))
        {
            session_close(session, j_response, UNPROCESSABLE_ENTITY,
                          "Updating searched_blk_no failed!");
            return;
        }
        
        if (!current_bc_status
                ->update_acc(xmr_address, updated_acc))
        {
            session_close(session, j_response, UNPROCESSABLE_ENTITY,
                          "updating acc in search thread failed!");
            return;
        }

        j_response["request_fulfilled"] = true;
        j_response["status"]            = "Import will start shortly";
        j_response["new_request"]       = true;
        j_response["error"]             = "";

        session_close(session, j_response);

        return;
    }

    // payment fee is not zero, so we need to
    // ask for the payment. So we first get payment details
    // associated with the given account.

    auto xmr_payment = select_payment(*xmr_account);

    // something went wrong.
    if (!xmr_payment)
    {
        session_close(session, j_response, UNPROCESSABLE_ENTITY,
                      "Selecting payment details failed!");
        return;
    }

    // payment id is null
    if (xmr_payment->id == mysqlpp::null)
    {
        // no current payment record exist,
        // so we have to create new one.

        // payment request is new, so create its entry in
        // Payments table

        uint64_t payment_table_id {0};

        crypto::hash8 random_payment_id8 = crypto::rand<crypto::hash8>();

        string integrated_address =
                current_bc_status->get_account_integrated_address_as_str(
                        random_payment_id8);

        xmr_payment->account_id        = xmr_account->id.data;
        xmr_payment->payment_id        = pod_to_hex(random_payment_id8);
        xmr_payment->import_fee        = current_bc_status
                                            ->get_bc_setup().import_fee; // xmr
        xmr_payment->request_fulfilled = false;
        xmr_payment->tx_hash           = ""; // no tx_hash yet with the payment
        xmr_payment->payment_address   = integrated_address;

        if ((payment_table_id = xmr_accounts->insert(*xmr_payment)) == 0)
        {
            OMERROR << xmr_address.substr(0, 6)
                       + ": failed to create new payment record!";

            session_close(session, j_response, UNPROCESSABLE_ENTITY,
                          "Failed to create new payment record!");
            return;
        }

        // payment entry created

        j_response["payment_id"]        = payment_table_id;
        j_response["import_fee"]        = std::to_string(
                                            xmr_payment->import_fee);
        j_response["new_request"]       = true;
        j_response["request_fulfilled"]
                                        = bool {xmr_payment->request_fulfilled};
        j_response["payment_address"]   = xmr_payment->payment_address;
        j_response["status"]            = "Payment not yet received";
        j_response["error"]             = "";

        session_close(session, j_response);
        return;
    } // if (xmr_payment->id == mysqlpp::null)

    // payment id is not null, so it means that
    // we have already payment record in our db for that
    // account.

    bool request_fulfilled = bool {xmr_payment->request_fulfilled};

    if (request_fulfilled)
    {
        // if payment has been made, and we get new request to import txs
        // indicate that this is new requeest, but request was fulfiled.
        // front end should give proper message in this case

        j_response["request_fulfilled"] = request_fulfilled;
        j_response["import_fee"]        = 0;
        j_response["status"]            = "Wallet already imported or "
                                          "in the progress.";
        j_response["new_request"]       = false;
        j_response["error"]             = "";

        session_close(session, j_response);
        return;
    }

    // payment has not been yet done, so we are going
    // to check if it has just been done and update
    // db accordingly

    string integrated_address =
            current_bc_status->get_account_integrated_address_as_str(
                    xmr_payment->payment_id);

    if (integrated_address.empty())
    {
        session_close(session, j_response, UNPROCESSABLE_ENTITY,
                      "get_account_integrated_address_as_str failed!");
        return;
    }

    j_response["payment_id"]        = xmr_payment->payment_id;
    j_response["import_fee"]        = std::to_string(xmr_payment->import_fee);
    j_response["new_request"]       = false;
    j_response["request_fulfilled"] = request_fulfilled;
    j_response["payment_address"]   = integrated_address;
    j_response["status"]            = "Payment not yet received";

    string tx_hash_with_payment;

    // if payment has not yet been done
    // check if it has just been done now
    // if yes, mark it in mysql

    if(current_bc_status->search_if_payment_made(
            xmr_payment->payment_id,
            xmr_payment->import_fee,
            tx_hash_with_payment))
    {
        XmrPayment updated_xmr_payment = *xmr_payment;

        // updated values
        updated_xmr_payment.request_fulfilled = true;
        updated_xmr_payment.tx_hash           = tx_hash_with_payment;

        // save to mysql
        if (!xmr_accounts->update(*xmr_payment, updated_xmr_payment))
        {

            OMERROR << xmr_address.substr(0,6) +
                        "Updating payment db failed!\n";

            session_close(session, j_response, UNPROCESSABLE_ENTITY,
                          "Updating payment db failed!");
            return;
        }

        XmrAccount updated_acc = *xmr_account;

        updated_acc.scanned_block_height = 0;
        updated_acc.start_height = 0;

        // set scanned_block_height	to 0 to begin
        // scanning entire blockchain
        
        // @todo we will have race condition here
        // as we updated mysql here, but at the same time 
        // txsearch tread does the same thing
        // and just few lines blow we update_acc yet again

        if (!xmr_accounts->update(*xmr_account, updated_acc))
        {
            OMERROR << xmr_address.substr(0,6) +
                        "Updating scanned_block_height failed!\n";

            session_close(session, j_response, UNPROCESSABLE_ENTITY,
                          "Updating scanned_block_height failed!");
            return;
        }
        

        // if success, set acc to updated_acc;
        request_fulfilled = true;

        // change search blk number in the search thread
        if (!current_bc_status
                ->set_new_searched_blk_no(xmr_address, 0))
        {
            session_close(session, j_response, UNPROCESSABLE_ENTITY,
                          "updating searched_blk_no failed!");
            return;
        }

        if (!current_bc_status
                ->update_acc(xmr_address, updated_acc))
        {
            session_close(session, j_response, UNPROCESSABLE_ENTITY,
                          "updating acc in search thread failed!");
            return;
        }

        j_response["request_fulfilled"]
                = request_fulfilled;
        j_response["status"]
                = "Payment received. Thank you.";
        j_response["new_request"]       = true;
        j_response["error"]             = "";

    } // if(current_bc_status->search_if_payment_made(


    session_close(session, j_response);
}



void
OpenMoneroRequests::import_recent_wallet_request(
        const shared_ptr< Session > session, const Bytes & body)
{
    json j_response;
    json j_request;

    bool request_fulfilled {false};

    j_response["request_fulfilled"] = false;

    vector<string> requested_values {"address" , "view_key",
                                     "no_blocks_to_import"};

    if (!parse_request(body, requested_values,
                       j_request, j_response))
    {
        session_close(session, j_response, UNPROCESSABLE_ENTITY,
                      "Cant parse json body!");
        return;
    }

    string xmr_address;
    string view_key;

    try
    {
        xmr_address = j_request["address"];
        view_key    = j_request["view_key"];
    }
    catch (json::exception const& e)
    {
        OMERROR << xmr_address.substr(0,6) 
                << ": json exception: " << e.what();
        session_close(session, j_response, UNPROCESSABLE_ENTITY,
                      e.what());
        return;
    }

    uint64_t no_blocks_to_import {1000};

    try
    {
        no_blocks_to_import
                = boost::lexical_cast<uint64_t>(
                    j_request["no_blocks_to_import"].get<string>());
    }
    catch (std::exception const& e)
    {
        string msg = "Cant parse "
                + j_request["no_blocks_to_import"].get<string>()
                + " into number";

        session_close(session, j_response, UNPROCESSABLE_ENTITY,
                      msg);
        return;
    }
    
    // get account from mysql db if exists
    auto xmr_account = select_account(xmr_address, view_key);

    if (!xmr_account)
    {
        // if creation failed, just close the session
        session_close(session, j_response, UNPROCESSABLE_ENTITY,
                      "The account login or creation failed!");
        return;
    }

    // make sure that we dont import more that the maximum alowed no of blocks
    no_blocks_to_import = std::min(no_blocks_to_import,
                                   current_bc_status->get_bc_setup()
                                   .max_number_of_blocks_to_import);

    auto current_blkchain_height
            = current_bc_status->get_current_blockchain_height();

    no_blocks_to_import
            = std::min(no_blocks_to_import,
                      current_blkchain_height);

    XmrAccount& acc = *xmr_account;

    XmrAccount updated_acc = acc;

    // make sure scanned_block_height is larger than
    // no_blocks_to_import so we dont
    // end up with overflowing uint64_t.
    if (updated_acc.scanned_block_height < no_blocks_to_import)
    {
        session_close(session, j_response, UNPROCESSABLE_ENTITY,
                      "scanned_block_height < no_blocks_to_import!");
        return;
    }

    // if import fee is zero, than scanned_block_height will
    // be automatically set to zero. But in case someone does
    // not want to imporot from scrach, we set it here to
    // current blockchain height
    auto import_fee = current_bc_status->get_bc_setup().import_fee;

    if (import_fee == 0)
        updated_acc.scanned_block_height = current_blkchain_height;

    // repetead calls to import_recent_wallet_request will be
    // moving the scanning backward.
    // not sure yet if any protection is needed to
    // make sure that a user does not
    // go back too much back by importing his/hers
    // wallet multiple times in a row.
    updated_acc.scanned_block_height
            = updated_acc.scanned_block_height - no_blocks_to_import;

    if (!xmr_accounts->update(acc, updated_acc))
    {
        session_close(session, j_response, UNPROCESSABLE_ENTITY,
                      "Updating account failed!");
        return;
    }

    // change search blk number in the search thread
    if (!current_bc_status
            ->set_new_searched_blk_no(xmr_address,
                        updated_acc.scanned_block_height))
    {
        session_close(session, j_response, UNPROCESSABLE_ENTITY,
                      "Updating searched_blk_no failed!");
        return;
    }

    if (!current_bc_status
            ->update_acc(xmr_address, updated_acc))
    {
        session_close(session, j_response, UNPROCESSABLE_ENTITY,
                      "updating acc in search thread failed!");
        return;
    }
        
    j_response["request_fulfilled"] = true; 
    j_response["status"]  = "Updating account with for"
                            " importing recent txs successeful.";

    string response_body = j_response.dump();

    auto response_headers = make_headers({{ "Content-Length",
                                            std::to_string(
                                                    response_body.size())}});

    session->close( OK, response_body, response_headers);
}



void
OpenMoneroRequests::get_tx(
        const shared_ptr< Session > session, const Bytes & body)
{
    json j_response;
    json j_request;

    vector<string> requested_values {"address" , "view_key", "tx_hash"};

    if (!parse_request(body, requested_values, j_request, j_response))
    {
        session_close(session, j_response);
        return;
    }

    string xmr_address;
    string view_key;
    string tx_hash_str;

    try
    {
        xmr_address = j_request["address"];
        view_key    = j_request["view_key"];
        tx_hash_str = j_request["tx_hash"];
    }
    catch (json::exception const& e)
    {
        OMWARN << "json exception: " << e.what();
        session_close(session, j_response);
        return;
    }

    j_response["status"] = "error";
    j_response["error"]  = "Some error occured";


    crypto::hash tx_hash;

    if (!hex_to_pod(tx_hash_str, tx_hash))
    {
        cerr << "Cant parse tx hash! : " << tx_hash_str  << '\n';
        j_response["status"] = "Cant parse tx hash! : " + tx_hash_str;
        session_close(session, j_response);
        return;
    }

    transaction tx;

    uint64_t default_timestamp {0};

    bool tx_found {false};

    bool tx_in_mempool {false};

    if (!current_bc_status->get_tx(tx_hash, tx))
    {
        // if tx not found in the blockchain, check if its in mempool

                // recieved_time, tx
        vector<pair<uint64_t, transaction>> mempool_txs =
                current_bc_status->get_mempool_txs();

        //cout << "serach mempool" << endl;

        for (auto const& mtx: mempool_txs)
        {
            //cout << (get_transaction_hash(mtx.second)) << '\n';

            if (get_transaction_hash(mtx.second) == tx_hash)
            {
                tx = mtx.second;
                tx_found = true;
                tx_in_mempool = true;
                default_timestamp = mtx.first;
                break;
            }
        }
    }
    else
    {
        tx_found = true;
    }

    if (!tx_found)
    {
        // if creation failed, just close the session
        session_close(session, j_response, UNPROCESSABLE_ENTITY,
                      " Cant get tx details for" + tx_hash_str);
        return;
    }

    // return tx hash. can be used to check if we acctually
    // delivered the tx that was requested
    j_response["tx_hash"]  = pod_to_hex(tx_hash);

    j_response["pub_key"]  = pod_to_hex(
                xmreg::get_tx_pub_key_from_received_outs(tx));


    bool coinbase = is_coinbase(tx);

    j_response["coinbase"] = coinbase;

    // key images of inputs
    vector<txin_to_key> input_key_imgs;

    // public keys and xmr amount of outputs
    vector<pair<txout_to_key, uint64_t>> output_pub_keys;

    uint64_t xmr_inputs;
    uint64_t xmr_outputs;
    uint64_t num_nonrct_inputs;
    uint64_t fee {0};
    uint64_t mixin_no;
    uint64_t size;

    // sum xmr in inputs and ouputs in the given tx
    array<uint64_t, 4> const& sum_data = xmreg::summary_of_in_out_rct(
            tx, output_pub_keys, input_key_imgs);

    xmr_outputs       = sum_data[0];
    xmr_inputs        = sum_data[1];
    mixin_no          = sum_data[2];
    num_nonrct_inputs = sum_data[3];

    j_response["xmr_outputs"]    = xmr_outputs;
    j_response["xmr_inputs"]     = xmr_inputs;
    j_response["mixin_no"]       = mixin_no;
    j_response["num_of_outputs"] = output_pub_keys.size();
    j_response["num_of_inputs"]  = input_key_imgs.size();
    j_response["tx_version"]     = tx.version;
    j_response["rct_type"]       = tx.rct_signatures.type;

    if (!coinbase &&  tx.vin.size() > 0)
    {
        // check if not miner tx
        // i.e., for blocks without any user transactions
        if (tx.vin.at(0).type() != typeid(txin_gen))
        {
            // get tx fee
            fee = get_tx_fee(tx);
        }
    }

    j_response["fee"] = fee;

    // get tx size in bytes
    size = get_object_blobsize(tx);

    j_response["size"] = size;

    // to be field later on using data from OutputInputIdentification
    j_response["total_sent"] = "0";
    j_response["total_received"] = "0";

    int64_t tx_height {-1};

    int64_t no_confirmations {-1};

    if (current_bc_status->get_tx_block_height(tx_hash, tx_height))
    {
        // get the current blockchain height. Just to check
        uint64_t bc_height = get_current_blockchain_height();

        no_confirmations = bc_height - tx_height;
    }

    // Class that is responsible for identification of our outputs
    // and inputs in a given tx.

    j_response["payment_id"] = string {};
    j_response["timestamp"]  = default_timestamp;

    address_parse_info address_info;
    secret_key viewkey;

    MicroCoreAdapter mcore_addapter {current_bc_status.get()};

    // to get info about recived xmr in this tx, we calculate it from
    // scrach, i.e., search for outputs. We could get this info
    // directly from the database, but doing it again here, is a good way
    // to double check tx data in the frontend, and also maybe try doing
    // it differently than before. Its not great, since we reinvent
    // the wheel
    // but its worth double checking
    // the mysql data, and also allows for new
    // implementation in the frontend.
    if (current_bc_status->get_xmr_address_viewkey(
                xmr_address, address_info, viewkey))
    {
        auto coreacc = make_account(xmr_address, view_key);

        if (!coreacc)
        {
            // if creation failed, just close the session
            session_close(session, j_response, UNPROCESSABLE_ENTITY,
                          "Cant create coreacc for " + xmr_address);
            return;
        }
    
        auto identifier = make_identifier(
                                tx, 
                                make_unique<Output>(coreacc.get()));

        identifier.identify();
    
        auto const& outputs_identified 
                = identifier.get<Output>()->get();

        auto total_received = calc_total_xmr(outputs_identified);

        j_response["total_received"] = std::to_string(total_received);

        json j_spent_outputs = json::array();

        // to get spendings, we need to have our key_images. but
        // the backend does not have spendkey, so it cant determine
        // which key images are really ours or not. this is the task
        // for the frontend. however, backend can only provide guesses and
        // nessessery data to the frontend to filter out incorrect
        // guesses.
        //
        // for input identification, we will use our mysql. its just much
        // faster to use it here, than before. but first we need to
        // get account id of the user asking for tx details.

        // a placeholder for exciting or new account data
        XmrAccount acc;


        // select this account if its existing one
        if (xmr_accounts->select(xmr_address, acc))
        {
            // if user exist, get tx data from database
            // this will work only for tx in the blockchain,
            // not those in the mempool.

            if (!tx_in_mempool)
            {
                // if not in mempool, but in blockchain, just
                // get data aout key images from the mysql

                XmrTransaction xmr_tx;

                if (xmr_accounts->tx_exists(
                            acc.id.data, tx_hash_str, xmr_tx))
                {
                    j_response["payment_id"] = xmr_tx.payment_id;
                    j_response["timestamp"]
                            = static_cast<uint64_t>(xmr_tx.timestamp*1e3);

                    vector<XmrInput> inputs;

                    if (xmr_accounts->select_for_tx(
                                xmr_tx.id.data, inputs))
                    {
                        json j_spent_outputs = json::array();

                        uint64_t total_spent {0};

                        for (XmrInput input: inputs)
                        {
                            XmrOutput out;

                            if (xmr_accounts
                                    ->select_by_primary_id(
                                        input.output_id, out))
                            {
                                total_spent += input.amount;

                                j_spent_outputs.push_back({
                                      {"amount"     , std::to_string(input.amount)},
                                      {"key_image"  , input.key_image},
                                      {"tx_pub_key" , out.tx_pub_key},
                                      {"out_index"  , out.out_index},
                                      {"mixin"      , out.mixin}});
                            }

                        } // for (XmrInput input: inputs)

                        j_response["total_sent"]    = std::to_string(total_spent);

                        j_response["spent_outputs"] = j_spent_outputs;

                    } // if (xmr_accounts->select_inputs_

                }  // if (xmr_accounts->tx_exists(acc.id

            } // if (!tx_in_mempool)
            else
            {
                // if tx in mempool, mysql will not have this tx, so
                // we cant pull info about key images from mysql for this tx.

                // we have to redo this info from basically from scrach.

                unordered_map<public_key, uint64_t> known_outputs_keys;

                if (current_bc_status->get_known_outputs_keys(
                        xmr_address, known_outputs_keys))
                {
                    // we got known_outputs_keys from the search thread.
                    // so now we can use OutputInputIdentification to
                    // get info about inputs.

                    // Class that is resposnible for idenficitaction
                    // of our outputs
                    // and inputs in a given tx.

                    auto identifier = make_identifier(tx, 
                                    make_unique<Input>(coreacc.get(), 
                                                       &known_outputs_keys, 
                                                       &mcore_addapter));
                    identifier.identify();
    
                    
                    auto const& inputs_identfied 
                        = identifier.get<Input>()->get();

                    json j_spent_outputs = json::array();

                    uint64_t total_spent {0};

                    for (auto& in_info: inputs_identfied)
                    {

                        // need to get output info from mysql, as we need
                        // to know output's amount, its orginal
                        // tx public key and its index in that tx
                        XmrOutput out;

                        string out_pub_key
                                = pod_to_hex(in_info.out_pub_key);

                        if (xmr_accounts->output_exists(out_pub_key, out))
                        {
                            total_spent += out.amount;

                            j_spent_outputs.push_back({
                                      {"amount"     , std::to_string(in_info.amount)},
                                      {"key_image"  , pod_to_hex(in_info.key_img)},
                                      {"tx_pub_key" , out.tx_pub_key},
                                      {"out_index"  , out.out_index},
                                      {"mixin"      , out.mixin}});
                        }

                    } //  for (auto& in_info: oi_identification

                    j_response["total_sent"]    = std::to_string(total_spent);

                    j_response["spent_outputs"] = j_spent_outputs;

                } //if (current_bc_status->get_known_outputs_keys(
                  //    xmr_address, known_outputs_keys))

            } //  else

        } //  if (xmr_accounts->select(xmr_address, acc))

    } //  if (current_bc_status->get_xmr_add

    j_response["tx_height"]         = tx_height;
    j_response["no_confirmations"]  = no_confirmations;
    j_response["status"]            = "OK";
    j_response["error"]             = "";

    string response_body = j_response.dump();

    auto response_headers = make_headers({{ "Content-Length",
                                            to_string(response_body.size())}});

    session->close( OK, response_body, response_headers);
}


void
OpenMoneroRequests::get_version(
        const shared_ptr< Session > session,
        const Bytes & body)
{

    (void) body;

    json j_response {
        {"last_git_commit_hash", string {GIT_COMMIT_HASH}},
        {"last_git_commit_date", string {GIT_COMMIT_DATETIME}},
        {"git_branch_name"     , string {GIT_BRANCH_NAME}},
        {"monero_version_full" , string {MONERO_VERSION_FULL}},
        {"api"                 , OPENMONERO_RPC_VERSION},
        {"testnet"             , current_bc_status->get_bc_setup().net_type
                    == network_type::TESTNET},
        {"network_type"        , current_bc_status->get_bc_setup().net_type},
        {"blockchain_height"   , get_current_blockchain_height()}
    };

    string response_body = j_response.dump();

    auto response_headers = make_headers({{ "Content-Length",
                                            to_string(response_body.size())}});

    session->close( OK, response_body, response_headers);
}


shared_ptr<Resource>
OpenMoneroRequests::make_resource(
        function< void (OpenMoneroRequests&, const shared_ptr< Session >,
                        const Bytes& ) > handle_func,
        const string& path)
{
    auto a_request = std::bind(handle_func, *this,
                               std::placeholders::_1,
                               std::placeholders::_2);

    shared_ptr<Resource> resource_ptr = make_shared<Resource>();

    resource_ptr->set_path(path);
    resource_ptr->set_method_handler( "OPTIONS", generic_options_handler);
    resource_ptr->set_method_handler( "POST"   , handel_(a_request) );

    return resource_ptr;
}


void
OpenMoneroRequests::generic_options_handler(
        const shared_ptr< Session > session )
{
    const auto request = session->get_request( );

    size_t content_length = request->get_header( "Content-Length", 0);

    session->fetch(content_length,
                   [](const shared_ptr< Session > session,
                   const Bytes &)
    {
        session->close( OK, string{}, make_headers());
    });
}


multimap<string, string>
OpenMoneroRequests::make_headers(
        const multimap<string, string>& extra_headers)
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
OpenMoneroRequests::print_json_log(const string& text, const json& j)
{
    cout << text << '\n' << j.dump(4) << endl;
}


string
OpenMoneroRequests::body_to_string(const Bytes & body)
{
    return string(reinterpret_cast<const char *>(body.data()), body.size());
}

json
OpenMoneroRequests::body_to_json(const Bytes & body)
{
    json j = json::parse(body_to_string(body));
    return j;
}


uint64_t
OpenMoneroRequests::get_current_blockchain_height() const
{
    return current_bc_status->get_current_blockchain_height();
}

bool
OpenMoneroRequests::login_and_start_search_thread(
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
            // newly created. Thus, we can start a thread
            // which will scan for transactions belonging to
            // that account, using its address and view key.
            // the thread will scan the blockchain for txs belonging
            // to that account and updated mysql database whenever it
            // will find something.
            //
            // The other client (i.e., a webbrowser) will query other
            // functions to retrieve
            // any belonging transactions in a loop.
            // Thus the thread does not need
            // to do anything except looking for tx and updating mysql
            // with relevant tx information

            if (!current_bc_status->search_thread_exist(acc.address))
            {
                std::unique_ptr<TxSearch> tx_search;

                try
                {
                    tx_search
                            = std::make_unique<TxSearch>(acc,
                                                         current_bc_status);
                }
                catch (std::exception const& e)
                {
                    OMERROR << "TxSearch construction faild.";
                    j_response = json {{"status", "error"},
                                       {"reason", "Failed to construct "
                                                  "TxSearch object"}};
                    return false;
                }


                if (current_bc_status->start_tx_search_thread(
                            acc, std::move(tx_search)))
                {
                    j_response["status"]      = "success";
                    j_response["new_address"] = false;

                    // thread has been started
                    // everything seems fine.

                    return true;
                }
            }
            else
            {
                j_response["status"]      = "success";
                j_response["new_address"] = false;

                // thread already exists
                // everything seems fine.

                return true;
            }

            j_response = json {{"status", "error"},
                               {"reason", "Failed created search "
                                          "thread for this account"}};
        }
        else
        {
            j_response = json {{"status", "error"},
                               {"reason", "Viewkey provided is incorrect"}};
        }
    }

    return false;
}


bool
OpenMoneroRequests::parse_request(
        const Bytes& body,
        vector<string>& values_map,
        json& j_request,
        json& j_response) const
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
        cerr << "OpenMoneroRequests::parse_request: " << e.what() << endl;

        j_response["status"] = "error";
        j_response["reason"] = "reqest json parsing failed";

        return false;
    }
}


boost::optional<XmrAccount>
OpenMoneroRequests::create_account(
        string const& xmr_address,
        string const& view_key,
        bool generated_locally) const
{
    boost::optional<XmrAccount> acc = XmrAccount{};

    if (xmr_accounts->select(xmr_address, *acc))
    {
        // if acc already exist, just return 
        // existing oneo
        
        OMINFO << xmr_address.substr(0,6) 
               <<  ": account already exists. "
               << "Return existing account";

        return acc;
    }

    uint64_t current_blockchain_height = get_current_blockchain_height();

    // initialize current blockchain timestamp with current time
    // in a moment we will try to get last block timestamp
    // to replace this value. But if it fails, we just use current
    // timestamp
    uint64_t current_blockchain_timestamp 
        = std::time(nullptr);

    // get last block so we have its timestamp when
    // createing the account
    block last_blk;

    if (current_bc_status->get_block(current_blockchain_height, last_blk))
    {
        if (last_blk.timestamp != 0)
            current_blockchain_timestamp = last_blk.timestamp;
    }

    DateTime blk_timestamp_mysql_format
            = XmrTransaction::timestamp_to_DateTime(
                current_blockchain_timestamp);

    //@todo setting up start_height and scanned_block_height
    //needs to be revisited as they are needed for importing
    //wallets. The simples way is when import is free and this
    //should already work. More problematic is how to set these
    //fields when import fee is non-zero. It depends
    //how mymonero is doing this. At the momemnt, I'm not sure.
    
    uint64_t start_height  =  current_blockchain_height;
    uint64_t scanned_block_height = current_blockchain_height;
    
    if (current_bc_status->get_bc_setup().import_fee == 0)
    {
    
        // accounts generated locally (using create account button)
        // will have start height equal to current blockchain height.
        // existing accounts, i.e., those imported ones, also called
        //  extenal ones  will have start_height of 0 to 
        //  indicated that they could
        // have been created years ago
    
        start_height  = generated_locally
                        ? current_blockchain_height : 0;
    
        // if scan block height is zero (for extranl wallets)
        // blockchain scanning starts immedietly.
        scanned_block_height = start_height;
    }

    // create new account
    acc = XmrAccount(
                   mysqlpp::null,
                   xmr_address,
                   make_hash(view_key),
                   scanned_block_height,
                   blk_timestamp_mysql_format,
                   start_height,
                   generated_locally);

    uint64_t acc_id {0};

    // insert the new account into the mysql
    if ((acc_id = xmr_accounts->insert(*acc)) == 0)
    {
        // if creating account failed
        OMERROR << xmr_address.substr(0,6) 
                << ": account creation failed: "
                << (*acc) ;

        return {};
    }

    // add acc database id
    acc->id = acc_id;
    
    // add also the view_key into acc object. its needs to be done
    // as we dont store viewkeys in the database
    acc->viewkey = view_key;

    return acc;
}

boost::optional<XmrAccount>
OpenMoneroRequests::select_account(
        string const& xmr_address,
        string const& view_key,
        bool create_if_notfound) const 
{
    boost::optional<XmrAccount> acc = XmrAccount{};

    if (!xmr_accounts->select(xmr_address, *acc))
    {
        OMINFO << xmr_address.substr(0,6) +
                   ": address does not exists";

        if (!create_if_notfound)
        {
            OMINFO << "create_if_notfound is false";
            return {};
        }

        // for this address
        if (!(acc = create_account(xmr_address, view_key)))
        {
            OMERROR << xmr_address.substr(0,6) 
                    << ": create_account failed";
            return {};
        }

        // once account has been created
        // make and start a search thread for it
        if (!make_search_thread(*acc))
        {
            OMERROR << xmr_address.substr(0,6) 
                    << ": make_search_thread failed";
            return {};
        }
    }

    // also need to check if view key matches
    string viewkey_hash = make_hash(view_key);

    if (viewkey_hash != acc->viewkey_hash)
    {
        OMWARN << xmr_address.substr(0,6) +
                   ": viewkey does not match " +
                   "the one in database!";
        return {};
    }

    return acc;
}

bool 
OpenMoneroRequests::make_search_thread(
        XmrAccount& acc) const 
{
    if (current_bc_status->search_thread_exist(acc.address))
    {
        return true;
    }

    std::unique_ptr<TxSearch> tx_search;

    try
    {
        tx_search = std::make_unique<TxSearch>(
                acc, current_bc_status);
    }
    catch (std::exception const& e)
    {
        OMERROR << acc.address.substr(0,6) 
            + ": txSearch construction faild.";
        return false;
    }

    return current_bc_status->start_tx_search_thread(
                acc, std::move(tx_search));
}

boost::optional<XmrPayment>
OpenMoneroRequests::select_payment(
        XmrAccount const& xmr_account) const
{
     vector<XmrPayment> xmr_payments;

     if (!xmr_accounts->select(xmr_account.id.data,
                               xmr_payments))
     {
         OMINFO << xmr_account.address.substr(0,6) +
                    ": no payment record found!";

         // so create empty record to be inserted into
         // db after.
         XmrPayment xmr_payment;
         xmr_payment.id = mysqlpp::null;

         return xmr_payment;
     }

     if (xmr_payments.size() > 1)
     {
         OMERROR << xmr_account.address.substr(0,6) +
                    ": more than one payment record found!";
         return {};
     }

     // if xmr_payments is empty it means
     // that the given account has no import
     // paymnet record created. so new
     // paymnet will be created
     if (xmr_payments.empty())
     {                  
         OMINFO << xmr_account.address.substr(0,6) +
                    ": no payment record found!";

         // so create empty record to be inserted into
         // db after.
         XmrPayment xmr_payment;
         xmr_payment.id = mysqlpp::null;

         return xmr_payment;
     }

     return xmr_payments.at(0);
}

void
OpenMoneroRequests::session_close(
        const shared_ptr< Session > session,
        json& j_response,
        int return_code,
        string error_msg) const
{
    if (return_code != OK)
    {
        j_response["Error"] = error_msg;
    }

    string response_body = j_response.dump();

    auto response_headers
            = make_headers({{ "Content-Length",
                              std::to_string(response_body.size())}});

    session->close( return_code,
                    response_body, response_headers);
}

}



