//
// Created by mwo on 5/11/15.
//

#include "MicroCore.h"


namespace xmreg
{
/**
 * The constructor is interesting, as
 * m_mempool and m_blockchain_storage depend
 * on each other.
 *
 * So basically m_mempool is initialized with
 * reference to Blockchain (i.e., Blockchain&)
 * and m_blockchain_storage is initialized with
 * reference to m_mempool (i.e., tx_memory_pool&)
 *
 * The same is done in cryptonode::core.
 */
MicroCore::MicroCore():      
        m_mempool(core_storage),
        core_storage(m_mempool),
        m_device {&hw::get_device("default")}
{

}


/**
 * Initialized the MicroCore object.
 *
 * Create BlockchainLMDB on the heap.
 * Open database files located in blockchain_path.
 * Initialize m_blockchain_storage with the BlockchainLMDB object.
 */
bool
MicroCore::init(const string& _blockchain_path, network_type nt)
{
    blockchain_path = _blockchain_path;

    nettype = nt;

    std::unique_ptr<BlockchainDB> db(new_db("lmdb"));

    if (db == nullptr)
    {
        cerr << "Attempted to use non-existent database type\n";
        return false;
    }

    try
    {
        // try opening lmdb database files
        db->open(blockchain_path, DBF_RDONLY);
    }
    catch (const std::exception& e)
    {
        cerr << "Error opening database: " << e.what();
        return false;
    }

    // check if the blockchain database
    // is successful opened
    if(!db->is_open())
        return false;

    // initialize Blockchain object to manage
    // the database.
    if (!core_storage.init(db.release(), nettype))
    {
        cerr << "Error opening database: core_storage->init(db, nettype)\n" ;
        return false;
    }

    if (!m_mempool.init())
    {
        cerr << "Error opening database: m_mempool.init()\n" ;
        return false;
    }

    return true;
}

/**
* Get m_blockchain_storage.
* Initialize m_blockchain_storage with the BlockchainLMDB object.
*/
Blockchain const&
MicroCore::get_core() const
{
    return core_storage;
}

tx_memory_pool const&
MicroCore::get_mempool() const
{
    return m_mempool;
}

    bool
MicroCore::get_block_from_height(const uint64_t& height, block& blk)
{

    try
    {
        blk = core_storage.get_db().get_block_from_height(height);
    }
    catch (const exception& e)
    {
        cerr << e.what() << endl;
        return false;
    }

    return true;
}



/**
 * Get transaction tx from the blockchain using it hash
 */
bool
MicroCore::get_tx(const crypto::hash& tx_hash, transaction& tx)
{
    if (core_storage.have_tx(tx_hash))
    {
        // get transaction with given hash
        tx = core_storage.get_db().get_tx(tx_hash);
    }
    else
    {
        cerr << "MicroCore::get_tx tx does not exist in blockchain: " << tx_hash << endl;
        return false;
    }


    return true;
}

hw::device* const
MicroCore::get_device() const
{
    return m_device;
}


}
