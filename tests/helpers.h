#pragma once

#include "src/UniversalIdentifier.hpp"

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

#define SPENDKEY_FROM_STRING(spendkey_str)                                   \
    crypto::secret_key spendkey;                                             \
    ASSERT_TRUE(xmreg::parse_str_secret_key(spendkey_str, spendkey));


#define TX_AND_ADDR_VIEWKEY_FROM_STRING(hex_tx, address_str, viewkey_str, net_type) \
         TX_FROM_HEX(hex_tx);                                                \
         ADDR_VIEWKEY_FROM_STRING(address_str, viewkey_str, net_type);


#define GET_RING_MEMBER_OUTPUTS(ring_member_data_hex)                        \
    MockGettingOutputs::ring_members_mock_map_t mock_ring_members_data;      \
    ASSERT_TRUE(xmreg::output_data_from_hex(ring_member_data_hex,            \
                                mock_ring_members_data));                    \
    MockGettingOutputs get_outputs(mock_ring_members_data);


#define GET_KNOWN_OUTPUTS(known_outputs_csv)                                \
    Input::known_outputs_t known_outputs;                                   \
    ASSERT_TRUE(check_and_adjust_path(known_outputs_csv));                  \
    ASSERT_TRUE(xmreg::populate_known_outputs_from_csv(                     \
                   known_outputs_csv, known_outputs));


#define MOCK_MCORE_GET_OUTPUT_KEY()                                         \
    auto mcore = std::make_unique<MockMicroCore>();                         \
    EXPECT_CALL(*mcore, get_output_key(_,_,_))                              \
                 .WillRepeatedly(Invoke(&get_outputs,                       \
                                         &MockGettingOutputs                \
                                            ::get_output_key));


