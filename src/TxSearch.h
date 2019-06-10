#pragma once

#include "db/MySqlAccounts.h"

#include <memory>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <unordered_map>

namespace xmreg
{

using namespace std;

using chrono::seconds;
using namespace literals::chrono_literals;

class XmrAccount;
class MySqlAccounts;

class TxSearchException: public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

class TxSearch
{

public:
    //                                         out_pk   , amount
    using known_outputs_t = std::unordered_map<public_key, uint64_t>;
    using addr_view_t = std::pair<address_parse_info, secret_key>;
    using pool_txs_t = std::vector<pair<uint64_t, transaction>>;

private:

    // how frequently update scanned_block_height in Accounts table
    //static constexpr uint64_t UPDATE_SCANNED_HEIGHT_INTERVAL = 5; // seconds

    // how long should the search thread be live after no request
    // are coming from the frontend. For example, when a user finishes
    // using the service.
    static seconds thread_search_life;

    // indicate that a thread loop should keep running
    std::atomic_bool continue_search {true};

    // this acctually indicates whether thread loop finished
    // its execution
    std::atomic_bool searching_is_ongoing {false};

    // marked true when we set new searched block value
    // from other thread. for example, when we import account
    // we set it to 0
    std::atomic_bool searched_block_got_updated {false};

    // to store last exception thrown in the search thread
    // using this, a main thread can get info what went wrong here
    std::exception_ptr eptr;

    mutex getting_eptr;
    mutex getting_known_outputs_keys;
    mutex access_acc;

    seconds last_ping_timestamp;

    atomic<uint64_t> searched_blk_no;

    // represents a row in mysql's Accounts table
    shared_ptr<XmrAccount> acc;

    // stores known output public keys.
    // used as a cash to fast look up of
    // our public keys in key images. Saves a lot of
    // mysql queries to Outputs table.
    //

    known_outputs_t known_outputs_keys;

    // this manages all mysql queries
    // its better to when each thread has its own mysql connection object.
    // this way if one thread crashes, it want take down
    // connection for the entire service
    std::shared_ptr<MySqlAccounts> xmr_accounts;

    std::shared_ptr<CurrentBlockchainStatus> current_bc_status;

    // address and viewkey for this search thread.
    address_parse_info address;
    secret_key viewkey;

    string address_prefix;

public:

    // make default constructor. useful in testing
    TxSearch() = default;

    TxSearch(XmrAccount const& _acc,
             std::shared_ptr<CurrentBlockchainStatus> _current_bc_status);

    virtual void
    operator()();

    virtual void
    stop();

    virtual void
    set_searched_blk_no(uint64_t new_value);

    virtual uint64_t
    get_searched_blk_no() const;

    virtual seconds
    get_current_timestamp() const;

    virtual void
    ping();

    virtual bool
    still_searching() const;

    virtual void
    populate_known_outputs();

    virtual known_outputs_t
    get_known_outputs_keys();

    virtual void
    update_acc(XmrAccount const& _acc);

    virtual void
    set_exception_ptr()
    {
        std::lock_guard<std::mutex> lck (getting_eptr);
        eptr = std::current_exception();
        stop();
    }

    virtual std::exception_ptr
    get_exception_ptr()
    {
        std::lock_guard<std::mutex> lck (getting_eptr);
        return eptr;
    }

    /**
     * Search for our txs in the mempool
     *
     * The method searches for our txs (outputs and inputs)
     * in the mempool. It does basically same what search method
     * The difference is that search method searches in a thread
     * in a blockchain. It does not scan for tx in mempool. This is because
     * it writes what it finds into database for permament storage.
     * However txs in mempool are not permament. Also since we want to
     * give the end user quick update on incoming/outging tx, this method
     * will be executed whenever frontend wants. By default it is every
     * 10 seconds.
     * Also since we dont write here anything to the database, we
     * return a json that will be appended to json produced by get_address_tx
     * and similar function. The outputs here cant be spent anyway. This is
     * only for end user information. The txs found here will be written
     * to database later on by TxSearch thread when they will be added
     * to the blockchain.
     *
     * we pass mempool_txs by copy because we want copy of mempool txs.
     * to avoid worrying about synchronizing threads
     *
     * @return json
     */
    virtual void
    find_txs_in_mempool(pool_txs_t mempool_txs,
                        json* j_transactions);

    virtual addr_view_t
    get_xmr_address_viewkey() const;
    
    virtual string
    get_viewkey() const;

    static void
    set_search_thread_life(seconds life_seconds);

    virtual bool
    delete_existing_tx_if_exists(string const& tx_hash);

    virtual ~TxSearch();

};



}
