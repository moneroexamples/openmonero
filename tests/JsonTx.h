#ifndef JSONTX_H
#define JSONTX_H

#include "../src/utils.h"

namespace xmreg
{

/**
 * @brief reprsents txs found in tests/res/tx folder
 *
 * Usef for unit testing and mocking tx data, which
 * otherwise would need to be fetched from blockchain
 *
*/
class JsonTx
{
public:

    struct output
    {
        uint64_t index {0};
        public_key pub_key;
        uint64_t amount {0};
    };

    struct account
    {
        address_parse_info address {};
        secret_key viewkey {};
        secret_key spendkey {};
        bool is_subaddress {false};
        network_type ntype;
        uint64_t amount {0};
        uint64_t change {0};
        vector<output> outputs;

        inline string
        address_str() const
        {
            return get_account_address_as_str(ntype,
                                              is_subaddress,
                                              address.address);
        }

        friend std::ostream&
        operator<<(std::ostream& os, account const& _account);
    };

    json jtx;

    transaction tx;
    crypto::hash tx_hash {0};                                                    \
    crypto::hash tx_prefix_hash {0};
    crypto::public_key tx_pub_key;
    crypto::hash payment_id {0};
    crypto::hash8 payment_id8 {0};
    crypto::hash8 payment_id8e {0};

    string jpath;
    network_type ntype;

    account sender;
    vector<account> recipients;

    explicit JsonTx(json _jtx);
    explicit JsonTx(string _path);

    // generate output which normaly is produced by
    // monero's get_output_tx_and_index and blockchain,
    // but here we use data from the tx json data file
    // need this for mocking blockchain calls in unit tests
    void
    get_output_tx_and_index(
            uint64_t const& amount,
            vector<uint64_t> const& offsets,
            vector<tx_out_index>& indices) const;

    // will use data from the json file
    // to set tx. Used for mocking this operation
    // that is normaly done using blockchain
    bool
    get_tx(crypto::hash const& tx_hash,
           transaction& tx) const;

private:
    void init();
    bool read_config();
    void populate_outputs(json const& joutputs, vector<output>& outs);

};

inline std::ostream&
operator<<(std::ostream& os, JsonTx::account const& _account)
{
    return os << _account.address_str();
}

bool
check_and_adjust_path(string& in_path);


boost::optional<JsonTx>
construct_jsontx(string tx_hash);

}


#endif // JSONTX_H
