#ifndef HELPERS_H
#define HELPERS_H

#include <string>

#define RAND_TX_HASH()                                      \
    crypto::hash tx_hash = crypto::rand<crypto::hash>();    \
    string tx_hash_str = pod_to_hex(tx_hash);               \

#define TX_FROM_HEX(hex_string)                                                 \
    transaction tx;                                                             \
    crypto::hash tx_hash;                                                       \
    crypto::hash tx_prefix_hash;                                                \
    ASSERT_TRUE(xmreg::hex_to_tx(hex_string, tx, tx_hash, tx_prefix_hash));     \
    string tx_hash_str = pod_to_hex(tx_hash);                                   \
    string tx_prefix_hash_str = pod_to_hex(tx_prefix_hash);

#define ACC_FROM_HEX(hex_address)                                               \
         xmreg::XmrAccount acc;                                                 \
         ASSERT_TRUE(this->xmr_accounts->select(hex_address, acc));


#define TX_AND_ACC_FROM_HEX(hex_tx, hex_address)                                \
         TX_FROM_HEX(hex_tx);                                                   \
         ACC_FROM_HEX(hex_address);



#endif // HELPERS_H
