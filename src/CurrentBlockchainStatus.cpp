//
// Created by mwo on 7/01/17.
//

#include "CurrentBlockchainStatus.h"


#include "tools.h"
#include "mylmdb.h"
#include "rpccalls.h"
#include "MySqlAccounts.h"
#include "TxSearch.h"

namespace xmreg
{



// initialize static variables
atomic<uint64_t>        CurrentBlockchainStatus::current_height{0};
string                  CurrentBlockchainStatus::blockchain_path{"/home/mwo/.blockchain/lmdb"};
string                  CurrentBlockchainStatus::deamon_url{"http:://127.0.0.1:18081"};
bool                    CurrentBlockchainStatus::testnet{false};
bool                    CurrentBlockchainStatus::is_running{false};
std::thread             CurrentBlockchainStatus::m_thread;
uint64_t                CurrentBlockchainStatus::refresh_block_status_every_seconds{60};
xmreg::MicroCore        CurrentBlockchainStatus::mcore;
cryptonote::Blockchain *CurrentBlockchainStatus::core_storage;
vector<transaction>     CurrentBlockchainStatus::mempool_txs;
string                  CurrentBlockchainStatus::import_payment_address;
string                  CurrentBlockchainStatus::import_payment_viewkey;
uint64_t                CurrentBlockchainStatus::import_fee {10000000000}; // 0.01 xmr
uint64_t                CurrentBlockchainStatus::spendable_age {10}; // default number in monero
account_public_address  CurrentBlockchainStatus::address;
secret_key              CurrentBlockchainStatus::viewkey;
map<string, shared_ptr<TxSearch>> CurrentBlockchainStatus::searching_threads;

void
CurrentBlockchainStatus::start_monitor_blockchain_thread()
{
    bool testnet = CurrentBlockchainStatus::testnet;


    if (!import_payment_address.empty() && !import_payment_viewkey.empty())
    {
        if (!xmreg::parse_str_address(
                import_payment_address,
                address,
                testnet))
        {
            cerr << "Cant parse address_str: "
                 << import_payment_address
                 << endl;
            return;
        }

        if (!xmreg::parse_str_secret_key(
                import_payment_viewkey,
                viewkey))
        {
            cerr << "Cant parse the viewkey_str: "
                 << import_payment_viewkey
                 << endl;
            return;
        }
    }

    if (!is_running)
    {
        m_thread = std::thread{[]()
                               {
                                   while (true)
                                   {
                                       current_height = get_current_blockchain_height();
                                       read_mempool();
                                       cout << "Check block height: " << current_height;
                                       cout << " no of mempool txs: " << mempool_txs.size();
                                       cout << endl;
                                       clean_search_thread_map();
                                       std::this_thread::sleep_for(
                                               std::chrono::seconds(
                                                       refresh_block_status_every_seconds));
                                   }
                               }};

        is_running = true;
    }
}


uint64_t
CurrentBlockchainStatus::get_current_blockchain_height()
{
    return xmreg::MyLMDB::get_blockchain_height(blockchain_path) - 1;
}

void
CurrentBlockchainStatus::set_blockchain_path(const string &path)
{
    blockchain_path = path;
}

void
CurrentBlockchainStatus::set_testnet(bool is_testnet)
{
    testnet = is_testnet;
}

bool
CurrentBlockchainStatus::init_monero_blockchain()
{
    // enable basic monero log output
    xmreg::enable_monero_log();

    // initialize mcore and core_storage
    if (!xmreg::init_blockchain(blockchain_path,
                                mcore, core_storage))
    {
        cerr << "Error accessing blockchain." << endl;
        return false;
    }

    return true;
}


bool
CurrentBlockchainStatus::is_tx_unlocked(uint64_t tx_blk_height)
{
    return (tx_blk_height + spendable_age > get_current_blockchain_height());
}

bool
CurrentBlockchainStatus::get_block(uint64_t height, block &blk)
{
    return mcore.get_block_from_height(height, blk);
}

bool
CurrentBlockchainStatus::get_block_txs(const block &blk, list <transaction> &blk_txs)
{
    // get all transactions in the block found
    // initialize the first list with transaction for solving
    // the block i.e. coinbase tx.
    blk_txs.push_back(blk.miner_tx);

    list <crypto::hash> missed_txs;

    if (!core_storage->get_transactions(blk.tx_hashes, blk_txs, missed_txs)) {
        cerr << "Cant get transactions in block: " << get_block_hash(blk) << endl;
        return false;
    }

    return true;
}

bool
CurrentBlockchainStatus::get_output_keys(const uint64_t& amount,
            const vector<uint64_t>& absolute_offsets,
            vector<cryptonote::output_data_t>& outputs)
{
    try
    {
        core_storage->get_db().get_output_key(amount,
                                              absolute_offsets,
                                              outputs);
    }
    catch (const OUTPUT_DNE& e)
    {
        cerr << "get_output_keys: " << e.what() << endl;
        return false;
    }

    return true;
}

bool
CurrentBlockchainStatus::get_amount_specific_indices(const crypto::hash& tx_hash,
                            vector<uint64_t>& out_indices)
{
    try
    {
        // this index is lmdb index of a tx, not tx hash
        uint64_t tx_index;

        if (core_storage->get_db().tx_exists(tx_hash, tx_index))
        {
            out_indices = core_storage->get_db()
                    .get_tx_amount_output_indices(tx_index);

            return true;
        }
    }
    catch(const exception& e)
    {
        cerr << e.what() << endl;
    }

    return false;
}

bool
CurrentBlockchainStatus::get_random_outputs(const vector<uint64_t>& amounts,
                   const uint64_t& outs_count,
                   vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount>& found_outputs)
{
    rpccalls rpc {deamon_url};

    string error_msg;

    if (!rpc.get_random_outs_for_amounts(amounts, outs_count, found_outputs, error_msg))
    {
        cerr << "rpc.get_random_outs_for_amounts failed" << endl;
        return false;
    }

    return true;
}

bool
CurrentBlockchainStatus::commit_tx(const string& tx_blob)
{
    rpccalls rpc {deamon_url};

    string error_msg;

    if (!rpc.commit_tx(tx_blob, error_msg))
    {
        cerr << "commit_tx failed" << endl;
        return false;
    }

    return true;
}

bool
CurrentBlockchainStatus::read_mempool()
{
    rpccalls rpc {deamon_url};

    string error_msg;

    std::lock_guard<std::mutex> lck (getting_mempool_txs);

    // clear current mempool txs vector
    // repopulate it with each execution of read_mempool()
    // not very efficient but good enough for now.
    mempool_txs.clear();

    // get txs in the mempool
    std::vector<tx_info> mempool_tx_info;

    if (!rpc.get_mempool(mempool_tx_info))
    {
        cerr << "Getting mempool failed " << endl;
        return false;
    }

    // if dont have tx_blob member, construct tx
    // from json obtained from the rpc call

    for (size_t i = 0; i < mempool_tx_info.size(); ++i)
    {
        // get transaction info of the tx in the mempool
        tx_info _tx_info = mempool_tx_info.at(i);

        crypto::hash mem_tx_hash = null_hash;

        if (hex_to_pod(_tx_info.id_hash, mem_tx_hash))
        {
            transaction tx;

            if (!xmreg::make_tx_from_json(_tx_info.tx_json, tx))
            {
                cerr << "Cant make tx from _tx_info.tx_json" << endl;
                return false;
            }

            if (_tx_info.id_hash != pod_to_hex(get_transaction_hash(tx)))
            {
                cerr << "Hash of reconstructed tx from json does not match "
                        "what we should get!"
                     << endl;

                return false;
            }

            mempool_txs.push_back(tx);

        } // if (hex_to_pod(_tx_info.id_hash, mem_tx_hash))

    } // for (size_t i = 0; i < mempool_tx_info.size(); ++i)

    return true;
}

vector<transaction>
CurrentBlockchainStatus::get_mempool_txs()
{
    std::lock_guard<std::mutex> lck (getting_mempool_txs);
    return mempool_txs;
}

bool
CurrentBlockchainStatus::search_if_payment_made(
        const string& payment_id_str,
        const uint64_t& desired_amount,
        string& tx_hash_with_payment)
{

    vector<transaction> txs_to_check = get_mempool_txs();

    uint64_t current_blockchain_height = current_height;

    // apend txs in last to blocks into the txs_to_check vector
    for (uint64_t blk_i = current_blockchain_height - 10;
         blk_i <= current_blockchain_height;
         ++blk_i)
    {
        // get block cointaining this tx
        block blk;

        if (!get_block(blk_i, blk)) {
            cerr << "Cant get block of height: " + to_string(blk_i) << endl;
            return false;
        }

        list <cryptonote::transaction> blk_txs;

        if (!get_block_txs(blk, blk_txs))
        {
            cerr << "Cant get transactions in block: " << to_string(blk_i) << endl;
            return false;
        }

        txs_to_check.insert(txs_to_check.end(), blk_txs.begin(), blk_txs.end());
    }

    for (transaction& tx: txs_to_check)
    {
        if (is_coinbase(tx))
        {
            // not interested in coinbase txs
            continue;
        }

        string tx_payment_id_str = get_payment_id_as_string(tx);

        if (payment_id_str != tx_payment_id_str)
        {
            // check tx having specific payment id only
            continue;
        }

        public_key tx_pub_key = xmreg::get_tx_pub_key_from_received_outs(tx);

        //          <public_key  , amount  , out idx>
        vector<tuple<txout_to_key, uint64_t, uint64_t>> outputs;

        outputs = get_ouputs_tuple(tx);

        // for each output, in a tx, check if it belongs
        // to the given account of specific address and viewkey

        // public transaction key is combined with our viewkey
        // to create, so called, derived key.
        key_derivation derivation;

        if (!generate_key_derivation(tx_pub_key, viewkey, derivation))
        {
            cerr << "Cant get derived key for: "  << "\n"
                 << "pub_tx_key: " << tx_pub_key << " and "
                 << "prv_view_key" << viewkey << endl;

            return false;
        }

        string tx_hash_str = pod_to_hex(get_transaction_hash(tx));



        uint64_t total_received {0};

        for (auto& out: outputs)
        {
            txout_to_key txout_k = std::get<0>(out);
            uint64_t amount = std::get<1>(out);
            uint64_t output_idx_in_tx = std::get<2>(out);

            // get the tx output public key
            // that normally would be generated for us,
            // if someone had sent us some xmr.
            public_key generated_tx_pubkey;

            derive_public_key(derivation,
                              output_idx_in_tx,
                              address.m_spend_public_key,
                              generated_tx_pubkey);

            // check if generated public key matches the current output's key
            bool mine_output = (txout_k.key == generated_tx_pubkey);

            if (mine_output)
            {
                total_received += amount;
            }
        }

        cout << " - payment id check in tx: "
             << tx_hash_str
             << " found: " << total_received << endl;

        if (total_received >= desired_amount)
        {
            // the payment has been made.
            tx_hash_with_payment = tx_hash_str;
            return true;
        }
    }

    return false;
}


string
CurrentBlockchainStatus::get_payment_id_as_string(const transaction& tx)
{
    crypto::hash payment_id = null_hash;
    crypto::hash8 payment_id8 = null_hash8;

    get_payment_id(tx, payment_id, payment_id8);

    string payment_id_str{""};

    if (payment_id != null_hash)
    {
        payment_id_str = pod_to_hex(payment_id);
    }
    else if (payment_id8 != null_hash8)
    {
        payment_id_str = pod_to_hex(payment_id8);
    }

    return payment_id_str;
}



bool
CurrentBlockchainStatus::start_tx_search_thread(XmrAccount acc)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (searching_threads.count(acc.address) > 0)
    {
        // thread for this address exist, dont make new one
        cout << "Thread exisist, dont make new one" << endl;
        return false;
    }

    // make a tx_search object for the given xmr account
    searching_threads[acc.address] = make_shared<TxSearch>(acc);

    // start the thread for the created object
    std::thread t1 {&TxSearch::search, searching_threads[acc.address].get()};
    t1.detach();

    return true;
}

bool
CurrentBlockchainStatus::ping_search_thread(const string& address)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (searching_threads.count(address) == 0)
    {
        // thread does not exist
        cout << "does not exist" << endl;
        return false;
    }

    searching_threads[address].get()->ping();

    return true;
}

bool
CurrentBlockchainStatus::set_new_searched_blk_no(const string& address, uint64_t new_value)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (searching_threads.count(address) == 0)
    {
        // thread does not exist
        cout << " thread does not exist" << endl;
        return false;
    }

    searching_threads[address].get()->set_searched_blk_no(new_value);

    return true;
}


void
CurrentBlockchainStatus::clean_search_thread_map()
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    for (auto st: searching_threads)
    {
        if (st.second->still_searching() == false)
        {
            cout << st.first << " still searching: " << st.second->still_searching() << endl;
            searching_threads.erase(st.first);
        }
    }
}



}