//
// Created by mwo on 7/01/17.
//

#include "CurrentBlockchainStatus.h"
#include "src/UniversalIdentifier.hpp"


#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "openmonero"

namespace xmreg
{

CurrentBlockchainStatus::CurrentBlockchainStatus(
        BlockchainSetup _bc_setup,
        std::unique_ptr<MicroCore> _mcore,
        std::unique_ptr<RPCCalls> _rpc,
        std::unique_ptr<TP::ThreadPool> _tp)
    : bc_setup {_bc_setup},
      mcore {std::move(_mcore)},
      rpc {std::move(_rpc)},
      thread_pool {std::move(_tp)}
{
    is_running = false;
    stop_blockchain_monitor_loop = false;
}

void
CurrentBlockchainStatus::monitor_blockchain()
{
    TxSearch::set_search_thread_life(
                bc_setup.search_thread_life);

    if (!is_running)
    {
       is_running = true;

       while (true)
       {
           if (stop_blockchain_monitor_loop)
           {
               stop_search_threads();
               clean_search_thread_map();
               OMINFO << "Breaking monitor_blockchain thread loop.";
               break;
           }

           //OMVLOG1 << "PoolQueue size: " 
           OMINFO << "PoolQueue size: " 
                   << TP::DefaultThreadPool::queueSize(); 

           update_current_blockchain_height();           

           read_mempool();

           OMINFO << "Current blockchain height: " << current_height
                  << ", pool size: " << mempool_txs.size() << " txs"
                  << ", no of TxSearch threads: " << thread_map_size(); 

           clean_search_thread_map();

           std::this_thread::sleep_for(
                       bc_setup.refresh_block_status_every);
       }

       is_running = false;
    }

    OMINFO << "Exiting monitor_blockchain thread loop.";
}

uint64_t
CurrentBlockchainStatus::get_current_blockchain_height()
{
    return current_height;
}

uint64_t
CurrentBlockchainStatus::get_hard_fork_version() const
{

    auto future_result = thread_pool->submit(
        [this](auto current_height) 
        {
            return this->mcore
                       ->get_hard_fork_version(
                               current_height);
        }, current_height.load());

    return future_result.get();
}

void
CurrentBlockchainStatus::update_current_blockchain_height()
{
    uint64_t tmp {0};

    // This rpc call not only gets the blockchain height
    // but it also serves as a "ping" into the Monero
    // deamon to keep the connection between the
    // openmonero backend and the deamon alive
    if (rpc->get_current_height(tmp))
    {
        current_height = tmp - 1;
        return;
    }

    OMERROR << "rpc->get_current_height() failed!";
}

bool
CurrentBlockchainStatus::init_monero_blockchain()
{
    // initialize the core using the blockchain path
    return mcore->init(bc_setup.blockchain_path, bc_setup.net_type);}

// taken from
// bool wallet2::is_transfer_unlocked(uint64_t unlock_time,
//uint64_t block_height) const
bool
CurrentBlockchainStatus::is_tx_unlocked(
        uint64_t unlock_time,
        uint64_t block_height,
        TxUnlockChecker const& tx_unlock_checker)
{
    return tx_unlock_checker.is_unlocked(bc_setup.net_type,
                                         current_height,
                                         unlock_time,
                                         block_height);
}


bool
CurrentBlockchainStatus::get_block(uint64_t height, block& blk)
{
    
    auto future_result = thread_pool->submit(
            [this](auto height, auto& blk) 
                -> bool 
            {
                return this->mcore
                           ->get_block_from_height(height, blk);
            }, height, std::ref(blk));

    return future_result.get();
}

vector<block>
CurrentBlockchainStatus::get_blocks_range(
        uint64_t const& h1, uint64_t const& h2)
{
    auto future_result = thread_pool->submit(
            [this](auto h1, auto h2) 
                -> vector<block>
            {
                try 
                {
                    return this->mcore->get_blocks_range(h1, h2);
                }
                catch (BLOCK_DNE& e)
                {
                    OMERROR << e.what();
                    return {};
                }
            }, h1, h2);

    return future_result.get();
}

bool
CurrentBlockchainStatus::get_block_txs(
        const block &blk,
        vector<transaction>& blk_txs,
        vector<crypto::hash>& missed_txs)
{
    // get all transactions in the block found
    // initialize the first list with transaction for solving
    // the block i.e. coinbase tx.
    blk_txs.push_back(blk.miner_tx);

    auto future_result = thread_pool->submit(
            [this](auto const& blk, 
                   auto& blk_txs, auto& missed_txs) 
                -> bool
            {
                if (!this->mcore->get_transactions(
                            blk.tx_hashes, blk_txs, 
                            missed_txs))
                {
                    OMERROR << "Cant get transactions in block: "
                            << get_block_hash(blk);

                    return false;
                }

                return true;
            }, std::cref(blk), std::ref(blk_txs),
               std::ref(missed_txs));

    return future_result.get();
}


bool
CurrentBlockchainStatus::get_txs(
        vector<crypto::hash> const& txs_to_get,
        vector<transaction>& txs,
        vector<crypto::hash>& missed_txs)
{

    auto future_result = thread_pool->submit(
            [this](auto const& txs_to_get, 
                auto& txs, auto& missed_txs)
                -> bool
            {
                if (!this->mcore->get_transactions(
                            txs_to_get, txs, missed_txs))
                {
                    OMERROR << "CurrentBlockchainStatus::get_txs: "
                               "cant get transactions!";
                    return false;
                }

                return true;

            }, std::cref(txs_to_get), std::ref(txs), 
               std::ref(missed_txs));

    return future_result.get();
}

bool
CurrentBlockchainStatus::tx_exist(const crypto::hash& tx_hash)
{
    auto future_result = thread_pool->submit(
            [this](auto const& tx_hash) -> bool
            {
                return this->mcore->have_tx(tx_hash);
            }, std::cref(tx_hash));

    return future_result.get();
}

bool
CurrentBlockchainStatus::tx_exist(
        const crypto::hash& tx_hash,
        uint64_t& tx_index)
{
    auto future_result = thread_pool->submit(
            [this](auto const& tx_hash,
                   auto& tx_index) -> bool
            {
                return this->mcore->tx_exists(tx_hash, tx_index);
            }, std::cref(tx_hash), std::ref(tx_index));

    return future_result.get();
}


bool
CurrentBlockchainStatus::tx_exist(
        const string& tx_hash_str,
        uint64_t& tx_index)
{
    crypto::hash tx_hash;

    if (hex_to_pod(tx_hash_str, tx_hash))
    {
        return tx_exist(tx_hash, tx_index);
    }

    return false;
}
tx_out_index
CurrentBlockchainStatus::get_output_tx_and_index(
        uint64_t amount, uint64_t index) const
{
    auto future_result = thread_pool->submit(
        [this](auto amount, auto index) 
            -> tx_out_index 
        {
            return this->mcore
                ->get_output_tx_and_index(amount, index);
        }, amount, index);

    return future_result.get();
}

void
CurrentBlockchainStatus::get_output_tx_and_index(
            uint64_t amount,
            std::vector<uint64_t> const& offsets,
            std::vector<tx_out_index>& indices) const
{
    auto future_result = thread_pool->submit(
        [this](auto amount, 
               auto const& offsets, 
               auto& indices) 
            -> void
        {
            this->mcore
                ->get_output_tx_and_index(
                        amount, offsets, indices);
        }, amount, std::cref(offsets), 
           std::ref(indices));
}

bool
CurrentBlockchainStatus::get_tx_with_output(
        uint64_t output_idx, uint64_t amount,
        transaction& tx, uint64_t& output_idx_in_tx)
{

    tx_out_index tx_out_idx;

    try
    {
        // get pair pair<crypto::hash, uint64_t> where first is tx hash
        // and second is local index of the output i in that tx
        tx_out_idx = get_output_tx_and_index(amount, output_idx);
    }
    catch (const OUTPUT_DNE& e)
    {

        string out_msg = "Output with amount " + std::to_string(amount)
                         + "  and index " + std::to_string(output_idx) 
                         + " does not exist!";


        OMERROR << out_msg << ' ' << e.what();

        return false;
    }

    output_idx_in_tx = tx_out_idx.second;

    if (!get_tx(tx_out_idx.first, tx))
    {
        OMERROR << "Cant get tx: " << tx_out_idx.first;

        return false;
    }

    return true;
}
    
uint64_t 
CurrentBlockchainStatus::get_num_outputs(
        uint64_t amount) const
{
    auto future_result = thread_pool->submit(
            [this](auto const& amount) -> uint64_t
            {
                  return this->mcore->get_num_outputs(amount);
            }, std::cref(amount));

    return future_result.get();
}

bool
CurrentBlockchainStatus::get_output_keys(
        const uint64_t& amount,
        const vector<uint64_t>& absolute_offsets,
        vector<cryptonote::output_data_t>& outputs)
{
    auto future_result = thread_pool->submit(
            [this](auto const& amount, 
                auto const& absolute_offsets,
                auto& outputs) -> bool
            {
                try
                {
                    this->mcore->get_output_key(amount, 
                            absolute_offsets, outputs);
                    return true;
                }
                catch (...)
                {
                    OMERROR << "Can get_output_keys";
                }

                return false;

            }, std::cref(amount), std::cref(absolute_offsets), 
               std::ref(outputs));

    return future_result.get();
}


string
CurrentBlockchainStatus::get_account_integrated_address_as_str(
        crypto::hash8 const& payment_id8)
{
    return cryptonote::get_account_integrated_address_as_str(
            bc_setup.net_type,
            bc_setup.import_payment_address.address, payment_id8);
}

string
CurrentBlockchainStatus::get_account_integrated_address_as_str(
        string const& payment_id8_str)
{
    crypto::hash8 payment_id8;

    if (!hex_to_pod(payment_id8_str, payment_id8))
        return string {};

    return get_account_integrated_address_as_str(payment_id8);
}

bool
CurrentBlockchainStatus::get_amount_specific_indices(
        const crypto::hash& tx_hash,
        vector<uint64_t>& out_indices)
{
    auto future_result = thread_pool->submit(
        [this](auto const& tx_hash, auto& out_indices)
            -> bool
        {
            try
            {
                // this index is lmdb index of a tx, not tx hash
                uint64_t tx_index;

                if (this->mcore->tx_exists(tx_hash, tx_index))
                {
                    out_indices = this->mcore
                            ->get_tx_amount_output_indices(tx_index);

                    return true;
                }
            }
            catch(const exception& e)
            {
                OMERROR << e.what();
            }
            return false;

        }, std::cref(tx_hash), std::ref(out_indices));

    return future_result.get();
}

bool
CurrentBlockchainStatus::get_output_histogram(
        COMMAND_RPC_GET_OUTPUT_HISTOGRAM::request& req,
        COMMAND_RPC_GET_OUTPUT_HISTOGRAM::response& res) const
{
    auto future_result = thread_pool->submit(
        [this](auto const& req, auto& res) 
            -> bool 
        {
            return this->mcore
                    ->get_output_histogram(req, res);

        }, std::cref(req), std::ref(res));

    return future_result.get();
}

bool
CurrentBlockchainStatus::get_rct_output_distribution(
        std::vector<uint64_t>& rct_offsets) const
{

    if (!rpc->get_rct_output_distribution(rct_offsets))
    {
        OMERROR << "rpc->get_rct_output_distribution() "
                   "get_rct_output_distribution failed";
        return false;
    }

    return true;
}

unique_ptr<RandomOutputs>
CurrentBlockchainStatus::create_random_outputs_object(
        vector<uint64_t> const& amounts,
        uint64_t outs_count) const
{
    return make_unique<RandomOutputs>(
            this, amounts, outs_count);
}

bool
CurrentBlockchainStatus::get_random_outputs(
        vector<uint64_t> const& amounts,
        uint64_t outs_count,
        RandomOutputs::outs_for_amount_v& found_outputs)
{   
    unique_ptr<RandomOutputs> ro
            = create_random_outputs_object(amounts, outs_count);

    if (!ro->find_random_outputs())
    {
        OMERROR << "!ro.find_random_outputs()";
        return false;
    }

    found_outputs = ro->get_found_outputs();

    return true;
}
bool
CurrentBlockchainStatus::get_outs(
        COMMAND_RPC_GET_OUTPUTS_BIN::request const& req,
        COMMAND_RPC_GET_OUTPUTS_BIN::response& res) const
{
    auto future_result = thread_pool->submit(
        [this](auto const& req, auto& res)
            -> bool
        {
            if (!this->mcore->get_outs(req, res))
            {
                OMERROR << "mcore->get_outs(req, res) failed";
                return false;
            }
            return true;

        }, std::cref(req), std::ref(res));

    return future_result.get();
}

bool
CurrentBlockchainStatus::get_output(
        const uint64_t amount,
        const uint64_t global_output_index,
        COMMAND_RPC_GET_OUTPUTS_BIN::outkey& output_info)
{
    COMMAND_RPC_GET_OUTPUTS_BIN::request req;
    COMMAND_RPC_GET_OUTPUTS_BIN::response res;

    req.outputs.push_back(
            get_outputs_out {amount, global_output_index});

    if (!get_outs(req, res))
        return false;

    output_info = res.outs.at(0);

    return true;
}

uint64_t
CurrentBlockchainStatus::get_dynamic_per_kb_fee_estimate() const
{
    const double byte_to_kbyte_factor = 1024;

    uint64_t fee_per_byte = get_dynamic_base_fee_estimate();

    uint64_t fee_per_kB = static_cast<uint64_t>(
                fee_per_byte * byte_to_kbyte_factor);

    return fee_per_kB;
}

uint64_t
CurrentBlockchainStatus::get_dynamic_base_fee_estimate() const
{
    auto future_result = thread_pool->submit(
        [this]() -> uint64_t 
        {
            return this->mcore
                ->get_dynamic_base_fee_estimate(
                    FEE_ESTIMATE_GRACE_BLOCKS);
        });

    return future_result.get();
}

uint64_t
CurrentBlockchainStatus::get_tx_unlock_time(
        crypto::hash const& tx_hash) const
{
    
    auto future_result = thread_pool->submit(
        [this](auto const& tx_hash) -> uint64_t 
        {
            return this->mcore->get_tx_unlock_time(tx_hash);
        }, std::cref(tx_hash));

    return future_result.get();
}

bool
CurrentBlockchainStatus::commit_tx(
        const string& tx_blob,
        string& error_msg,
        bool do_not_relay)
{   

    if (!rpc->commit_tx(tx_blob, error_msg, do_not_relay))
    {
        OMERROR << "rpc->commit_tx() commit_tx failed";
        return false;
    }

    return true;
}

bool
CurrentBlockchainStatus::read_mempool()
{
    // get txs in the mempool

    mempool_txs_t local_mempool_txs;
    
    auto future_result = thread_pool->submit(
        [this](auto& local_mempool_txs)
            -> bool 
        {

            std::vector<transaction> txs;

            if (!this->mcore->get_mempool_txs(txs))
            {
                OMERROR << "Getting mempool failed ";
                return false;
            }

            for (size_t i = 0; i < txs.size(); ++i)
            {
                // get transaction info of the tx in the mempool
                auto const& tx = txs.at(i);

                tx_memory_pool::tx_details txd;

                txpool_tx_meta_t meta;

                if (!this->mcore->get_core().get_txpool_tx_meta(tx.hash, meta))
                {
                  OMERROR << "Failed to find tx in txpool";
                  return false;
                }

                local_mempool_txs.emplace_back(meta.receive_time, tx);

            } // for (size_t i = 0; i < mempool_tx_info.size(); ++i)

            return true;

        }, std::ref(local_mempool_txs));

    if (!future_result.get())
        return false;

    std::lock_guard<std::mutex> lck (getting_mempool_txs);

    mempool_txs = std::move(local_mempool_txs);

    return true;
}

CurrentBlockchainStatus::mempool_txs_t
CurrentBlockchainStatus::get_mempool_txs()
{
    std::lock_guard<std::mutex> lck (getting_mempool_txs);
    return mempool_txs;
}

//@todo search_if_payment_made is way too long!
bool
CurrentBlockchainStatus::search_if_payment_made(
        const string& payment_id_str,
        const uint64_t& desired_amount,
        string& tx_hash_with_payment)
{

    mempool_txs_t mempool_transactions = get_mempool_txs();

    uint64_t current_blockchain_height = get_current_blockchain_height();

//    cout << "current_blockchain_height: "
//         << current_blockchain_height << '\n';

    vector<transaction> txs_to_check;

    for (auto& mtx: mempool_transactions)
    {
        txs_to_check.push_back(mtx.second);
    }

    // apend txs in last to blocks into the txs_to_check vector
    for (uint64_t blk_i = current_blockchain_height - 10;
         blk_i <= current_blockchain_height;
         ++blk_i)
    {
        // get block cointaining this tx
        block blk;

        if (!get_block(blk_i, blk)) {
            OMERROR << "Cant get block of height: " << blk_i;
            return false;
        }

        vector<cryptonote::transaction> blk_txs;
        vector<crypto::hash> missed_txs;

        if (!get_block_txs(blk, blk_txs, missed_txs))
        {
            OMERROR << "Cant get transactions from block: " << blk_i;
            return false;
        }

        // combine mempool txs and txs from given number of
        // last blocks
        txs_to_check.insert(txs_to_check.end(),
                            blk_txs.begin(), blk_txs.end());
    }

    crypto::hash8 expected_payment_id;

    if (!hex_to_pod(payment_id_str, expected_payment_id))
    {
        OMERROR << "Cant convert payment id to pod: " << payment_id_str;
        return false;
    }


    for (auto&& tx: txs_to_check)
    {
        auto identifier = make_identifier(
                            tx, 
                            make_unique<Output>(
                                &bc_setup.import_payment_address,
                                &bc_setup.import_payment_viewkey),
                            make_unique<IntegratedPaymentID>(
                                &bc_setup.import_payment_address,
                                &bc_setup.import_payment_viewkey));
        identifier.identify();

        auto payment_id = identifier.get<IntegratedPaymentID>()->get();

        if (payment_id == expected_payment_id)
        {
            auto amount_paid = identifier.get<Output>()->get_total();

            if (amount_paid >= desired_amount)
            {

                string tx_hash_str = pod_to_hex(
                    get_transaction_hash(tx));

                OMINFO << " Payment id check in tx: " << tx_hash_str
                       << " found: " << amount_paid;

                // the payment has been made.
                tx_hash_with_payment = tx_hash_str;

                OMINFO << "Import payment done";

                return true;
            }
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


output_data_t
CurrentBlockchainStatus::get_output_key(
        uint64_t amount,
        uint64_t global_amount_index)
{
    
    auto future_result = thread_pool->submit(
        [this](auto amount, auto global_amount_index) 
            -> output_data_t
        {
            return this->mcore->get_output_key(
                    amount, global_amount_index);
        }, amount, global_amount_index);

    return future_result.get();
}

bool
CurrentBlockchainStatus::start_tx_search_thread(
        XmrAccount acc, std::unique_ptr<TxSearch> tx_search)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (search_thread_exist(acc.address))
    {
        // thread for this address exist, dont make new one
        //cout << "Thread exists, dont make new one\n";
        return true; // this is still OK, so return true.
    }

    try
    {
        // launch SearchTx thread for the given xmr account

        searching_threads.insert(
            {acc.address, ThreadRAII2<TxSearch>(std::move(tx_search))});

        OMINFO << acc.address.substr(0,6)
                  + ": TxSearch thread created.";
    }
    catch (const std::exception& e)
    {
        OMERROR << acc.address.substr(0,6)
                << ": Faild created a search thread: " 
                << e.what();

        return false;
    }

    return true;
}



bool
CurrentBlockchainStatus::ping_search_thread(const string& address)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (!search_thread_exist(address))
    {
        // thread does not exist
        OMERROR << "thread for " << address << " does not exist";
        return false;
    }

    get_search_thread(address).ping();

    return true;
}

bool
CurrentBlockchainStatus::get_searched_blk_no(const string& address,
                                             uint64_t& searched_blk_no)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (!search_thread_exist(address))
    {
        // thread does not exist
        OMERROR << "thread for " << address.substr(0,6) << " does not exist";
        return false;
    }

    searched_blk_no = get_search_thread(address).get_searched_blk_no();

    return true;
}

bool
CurrentBlockchainStatus::get_known_outputs_keys(
        string const& address,
        unordered_map<public_key, uint64_t>& known_outputs_keys)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (!search_thread_exist(address))
    {
        // thread does not exist
        OMERROR << "thread for " << address.substr(0,6) << " does not exist";
        return false;
    }


    known_outputs_keys = get_search_thread(address)
            .get_known_outputs_keys();

    return true;
}

bool
CurrentBlockchainStatus::search_thread_exist(const string& address)
{
    // no mutex here, as this will be executed
    // from other methods, which do use mutex.
    // so if you put mutex here, you will get into deadlock.
    return searching_threads.count(address) > 0;
}

bool
CurrentBlockchainStatus::search_thread_exist(
        string const& address, 
        string const& viewkey)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (!search_thread_exist(address))
        return false;

    return get_search_thread(address).get_viewkey() == viewkey;
}

bool
CurrentBlockchainStatus::get_xmr_address_viewkey(
        const string& address_str,
        address_parse_info& address,
        secret_key& viewkey)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (!search_thread_exist(address_str))
    {
        // thread does not exist
        OMERROR << "thread for " << address_str.substr(0,6)
                << " does not exist";
        return false;
    }

    address = get_search_thread(address_str)
            .get_xmr_address_viewkey().first;
    viewkey = get_search_thread(address_str)
            .get_xmr_address_viewkey().second;

    return true;
}

bool
CurrentBlockchainStatus::find_txs_in_mempool(
        const string& address_str,
        json& transactions)
{
    std::lock_guard<std::mutex> lck (getting_mempool_txs);

    if (searching_threads.count(address_str) == 0)
    {
        // thread does not exist
        OMERROR << "thread for "
                << address_str.substr(0,6) << " does not exist";
        return false;
    }

    get_search_thread(address_str)
            .find_txs_in_mempool(mempool_txs, &transactions);

    return true;
}

bool
CurrentBlockchainStatus::find_tx_in_mempool(
        crypto::hash const& tx_hash,
        transaction& tx)
{

    std::lock_guard<std::mutex> lck (getting_mempool_txs);

    for (auto const& mtx: mempool_txs)
    {
        const transaction &m_tx = mtx.second;

        if (get_transaction_hash(m_tx) == tx_hash)
        {
            tx = m_tx;
            return true;
        }
    }

    return false;
}

bool
CurrentBlockchainStatus::find_key_images_in_mempool(
        std::vector<txin_v> const& vin)
{
    std::lock_guard<std::mutex> lck (getting_mempool_txs);

    // perform exhostive search to check if any key image in vin vector
    // is in the mempool. This is used to check if a tx generated
    // by the frontend
    // is using any key images that area already in the mempool.
    for (auto const& kin: vin)
    {
        if(kin.type() != typeid(txin_to_key))
            continue;

        // get tx input key
        const txin_to_key& tx_in_to_key
                = boost::get<cryptonote::txin_to_key>(kin);

        for (auto const& mtx: mempool_txs)
        {
            transaction const& m_tx = mtx.second;

            vector<txin_to_key> input_key_imgs
                    = xmreg::get_key_images(m_tx);

            for (auto const& mki: input_key_imgs)
            {
                // if a matching key image found in the mempool
                if (mki.k_image == tx_in_to_key.k_image)
                    return true;
            }
        }
    }

    return false;
}


bool
CurrentBlockchainStatus::find_key_images_in_mempool(
        transaction const& tx)
{
    return find_key_images_in_mempool(tx.vin);
}


bool
CurrentBlockchainStatus::get_tx(
        crypto::hash const& tx_hash,
        transaction& tx)
{
    auto future_result = thread_pool->submit(
            [this](auto const& tx_hash,
                   auto& tx) -> bool
            {
                return this->mcore->get_tx(tx_hash, tx);
            }, std::cref(tx_hash), std::ref(tx));

    return future_result.get();
}


bool
CurrentBlockchainStatus::get_tx(
        string const& tx_hash_str,
        transaction& tx)
{
    crypto::hash tx_hash;

    if (!hex_to_pod(tx_hash_str, tx_hash))
    {
       return false;
    }

    return get_tx(tx_hash, tx);
}

bool
CurrentBlockchainStatus::get_tx_block_height(
        crypto::hash const& tx_hash,
        int64_t& tx_height)
{
    if (!tx_exist(tx_hash))
        return false;

    auto future_result = thread_pool->submit(
        [this](auto const tx_hash) 
            -> int64_t 
        {
            return this->mcore->get_tx_block_height(tx_hash);
        }, std::cref(tx_hash));

    tx_height = future_result.get();

    return true;
}

bool
CurrentBlockchainStatus::set_new_searched_blk_no(
        const string& address, uint64_t new_value)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (searching_threads.count(address) == 0)
    {
        // thread does not exist
        OMERROR << address.substr(0,6)
                   + ": set_new_searched_blk_no failed:"
                   " thread does not exist";
        return false;
    }

    get_search_thread(address).set_searched_blk_no(new_value);

    return true;
}

// this can be only used for updateding accont details
// of the same account. Cant be used to change the account
// to different addresses for example.
bool
CurrentBlockchainStatus::update_acc(
        const string& address, 
        XmrAccount const& _acc)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (searching_threads.count(address) == 0)
    {
        // thread does not exist
        OMERROR << address.substr(0,6)
                   + ": set_new_searched_blk_no failed:"
                   " thread does not exist";
        return false;
    }

    get_search_thread(address).update_acc(_acc);

    return true;
}

TxSearch&
CurrentBlockchainStatus::get_search_thread(string const& acc_address)
{
    // do not need locking mutex here, as this function should be only
    // executed from others that do lock the mutex already.
    auto it = searching_threads.find(acc_address);

    if (it == searching_threads.end())
    {
        OMERROR << "Search thread does not exisit for addr: "
                << acc_address;
        throw std::runtime_error("Trying to accesses "
                                 "non-existing search thread");
    }

    return searching_threads.find(acc_address)->second.get_functor();
}

size_t
CurrentBlockchainStatus::thread_map_size() 
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);
    return searching_threads.size();
}

void
CurrentBlockchainStatus::clean_search_thread_map()
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    for (auto it = searching_threads.begin(); 
            it != searching_threads.end();)
    {
        auto& st = *it;

        if (search_thread_exist(st.first)
                && st.second.get_functor().still_searching() == false)
        {
            // before erasing a search thread, check if there was any
            // exception thrown by it
            try
            {
                auto eptr = st.second.get_functor().get_exception_ptr();
                if (eptr != nullptr)
                    std::rethrow_exception(eptr);
            }
            catch (std::exception const& e)
            {
                OMERROR << "Error in search thread: " << e.what()
                        << ". It will be cleared.";
            }

            OMINFO << "Ereasing a search thread";
            it = searching_threads.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void
CurrentBlockchainStatus::stop_search_threads()
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    for (auto& st: searching_threads)
    {
        st.second.get_functor().stop();
    }
}

tuple<string, string, string>
CurrentBlockchainStatus::construct_output_rct_field(
        const uint64_t global_amount_index,
        const uint64_t out_amount)
{

    transaction random_output_tx;
    uint64_t output_idx_in_tx;

    // we got random outputs, but now we need to get rct data of those
    // outputs, because by default frontend creates ringct txs.

    if (!CurrentBlockchainStatus::get_tx_with_output(
            global_amount_index, out_amount,
            random_output_tx, output_idx_in_tx))
    {
        OMERROR << "cant get random output transaction";
        return make_tuple(string {}, string {}, string {});
    }

    // placeholder variable for ringct outputs info
    // that we need to save in database
    string rtc_outpk;
    string rtc_mask(64, '0');
    string rtc_amount(64, '0');


    //if (random_output_tx.version == 1)
    //{
        //rtc_outpk = "";
        //rtc_mask = "";
        //rtc_amount = "";
    //}
    //else
    //{

        
        //if (random_output_tx.rct_signatures.type > 0)
        //{
            //rtc_outpk  = pod_to_hex(random_output_tx.rct_signatures
                                    //.outPk[output_idx_in_tx].mask);
            //rtc_mask   = pod_to_hex(random_output_tx.rct_signatures
                                    //.ecdhInfo[output_idx_in_tx].mask);
            //rtc_amount = pod_to_hex(random_output_tx.rct_signatures
                                    //.ecdhInfo[output_idx_in_tx].amount);
        //}

        //if (random_output_tx.rct_signatures.type == 0)
        //{
            //rtc_outpk = "coinbase";
        //}
        //else if (random_output_tx.rct_signatures.type == 4)
        //{
            //rtc_amount = rtc_amount.substr(0,16);
        //}
    //}

    //cout << "random_outs : " << rtc_outpk << "," 
         //<< rtc_mask << ","<< rtc_amount << endl;



    if (random_output_tx.version > 1 && !is_coinbase(random_output_tx))
    {
        rtc_outpk  = pod_to_hex(random_output_tx.rct_signatures
                               .outPk[output_idx_in_tx].mask);
        rtc_mask   = pod_to_hex(random_output_tx.rct_signatures
                                .ecdhInfo[output_idx_in_tx].mask);
        rtc_amount = pod_to_hex(random_output_tx.rct_signatures
                                .ecdhInfo[output_idx_in_tx].amount);

    }
    else
    {
        //// for non ringct txs, we need to take it rct amount commitment
        //// and sent to the frontend. the mask is zero mask for those,
        //// as frontend will produce identy mask autmatically
        //// for non-ringct outputs

        output_data_t od = get_output_key(out_amount, global_amount_index);

        rtc_outpk  = pod_to_hex(od.commitment);

        //if (is_coinbase(random_output_tx))
        //{
            //// commenting this out. think its not needed.
            //// as this function provides keys for mixin outputs
            //// not the ones we actually spend.
            //// ringct coinbase txs are special. they have identity mask.
            //// as suggested by this code:
            //// https://github.com/monero-project/monero/blob/eacf2124b6822d088199179b18d4587404408e0f/src/wallet/wallet2.cpp#L893
            //// https://github.com/monero-project/monero/blob/master/src/blockchain_db/blockchain_db.cpp#L100
            //// rtc_mask   = pod_to_hex(rct::identity());
        //}

    }

    return make_tuple(rtc_outpk, rtc_mask, rtc_amount);
};

void
CurrentBlockchainStatus::init_txs_data_vector(
        vector<block> const& blocks,
        vector<crypto::hash>& txs_to_get,
        vector<txs_tuple_t>& txs_data)
{
    // get height of the first block
    uint64_t h1 = get_block_height(blocks[0]);

    for(uint64_t blk_i = 0; blk_i < blocks.size(); blk_i++)
    {
        block const& blk = blocks[blk_i];
        uint64_t blk_height = h1 + blk_i;

        // first insert miner_tx

        txs_data.emplace_back(blk_height, blk.timestamp, true);

        txs_to_get.push_back(get_transaction_hash(blk.miner_tx));

        // now insert hashes of regular txs to be fatched later
        // so for now, these txs are null pointers
        for (auto& tx_hash: blk.tx_hashes)
        {
            txs_data.emplace_back(blk_height, blk.timestamp, false);
            txs_to_get.push_back(tx_hash);
        }

    } // for(uint64_t blk_i = 0; blk_i < blocks.size(); blk_i++)
}

bool
CurrentBlockchainStatus::get_txs_in_blocks(
        vector<block> const& blocks,
        vector<crypto::hash>& txs_hashes,
        vector<transaction>& txs,
        vector<txs_tuple_t>& txs_data)
{

    // initialize vectors of txs hashes, block heights
    // and whethere or not txs are coninbase
    init_txs_data_vector(blocks, txs_hashes, txs_data);

    // fetch all txs from the blocks that we are
    // analyzing in this iteration
    vector<crypto::hash> missed_txs;

    if (!CurrentBlockchainStatus::get_txs(txs_hashes, txs, missed_txs)
            || !missed_txs.empty()
            || (txs_hashes.size() != txs.size()))
    {
        OMERROR << "Cant get transactions in blocks";
        return false;
    }

    (void) missed_txs;

    return true;
}



    
MicroCoreAdapter::MicroCoreAdapter(CurrentBlockchainStatus* _cbs)
: cbs {_cbs}
{}

uint64_t 
MicroCoreAdapter::get_num_outputs(uint64_t amount) const
{
    return cbs->get_num_outputs(amount);
}

void 
MicroCoreAdapter::get_output_key(uint64_t amount,
                  vector<uint64_t> const& absolute_offsets,
                  vector<cryptonote::output_data_t>& outputs) 
                   const 
{
    cbs->get_output_keys(amount, absolute_offsets, outputs);
}

void
MicroCoreAdapter::get_output_tx_and_index(
            uint64_t amount,
            std::vector<uint64_t> const& offsets,
            std::vector<tx_out_index>& indices) 
                const 
{
    cbs->get_output_tx_and_index(amount, offsets, indices);
}

bool
MicroCoreAdapter::get_tx(crypto::hash const& tx_hash, transaction& tx) 
        const
{
    return cbs->get_tx(tx_hash, tx);
}


}

