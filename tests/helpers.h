#pragma once

#include <string>

#define RAND_TX_HASH()                                      \
    crypto::hash tx_hash = crypto::rand<crypto::hash>();    \
    string tx_hash_str = pod_to_hex(tx_hash);               \

#define TX_FROM_HEX(hex_string)                                              \
    transaction tx;                                                          \
    crypto::hash tx_hash;                                                    \
    crypto::hash tx_prefix_hash;                                             \
    ASSERT_TRUE(xmreg::hex_to_tx(hex_string, tx, tx_hash, tx_prefix_hash));  \
    string tx_hash_str = pod_to_hex(tx_hash);                                \
    string tx_prefix_hash_str = pod_to_hex(tx_prefix_hash);                  \
    crypto::public_key tx_pub_key = xmreg::get_tx_pub_key_from_received_outs(tx);    \
    string tx_pub_key_str = pod_to_hex(tx_pub_key);                          \
    bool is_tx_coinbase = cryptonote::is_coinbase(tx);

#define ACC_FROM_HEX(hex_address)                                            \
         xmreg::XmrAccount acc;                                              \
         ASSERT_TRUE(this->xmr_accounts->select(hex_address, acc));


#define TX_AND_ACC_FROM_HEX(hex_tx, hex_address)                             \
         TX_FROM_HEX(hex_tx);                                                \
         ACC_FROM_HEX(hex_address);

#define ADDR_VIEWKEY_FROM_STRING(address_str, viewkey_str, net_type)         \
    address_parse_info address;                                              \
    crypto::secret_key viewkey;                                              \
    ASSERT_TRUE(xmreg::addr_and_viewkey_from_string(                         \
                    address_str, viewkey_str,                                \
                    net_type, address, viewkey));




