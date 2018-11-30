#ifndef JSONTX_H
#define JSONTX_H

#include "../src/tools.h"

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

    struct account
    {
        address_parse_info address {};
        secret_key viewkey {};
        secret_key spendkey {};
        uint64_t amount {0};
        uint64_t change {0};
    };

    json jtx;

    transaction tx;
    crypto::hash tx_hash;                                                    \
    crypto::hash tx_prefix_hash;
    crypto::public_key tx_pub_key;

    string jpath;
    network_type ntype;

    account sender;
    vector<account> recipients;

    explicit JsonTx(json _jtx);
    explicit JsonTx(string _path);

private:
    void init();
    bool read_config();

};

bool
check_and_adjust_path(string& in_path);


boost::optional<JsonTx>
construct_jsontx(string tx_hash);


}


#endif // JSONTX_H
