#pragma once

#include "om_log.h"
#include "OutputInputIdentification.h"

#include <map>
#include <utility>

namespace xmreg
{


class PaymentSearcherException: public std::runtime_error
{
    using std::runtime_error::runtime_error;
};


/**
 * Class for searching transactions having given payment_id
 * This is primarly used to check if a payment was made
 * for importing wallet
 */
template<typename Hash_T>
class PaymentSearcher
{
    // null hash value for hash8 or regular hash
    using null_hash_T = boost::value_initialized<Hash_T>;

public:
    PaymentSearcher(
            address_parse_info const& _address_info,
            secret_key const& _view_key,
            MicroCore* const _mcore):
          address_info {_address_info},
          view_key {_view_key},
          mcore {_mcore}
    {}


    template<template<typename T,
                      typename A = std::allocator<T>>
             class Cont_T>
    std::pair<uint64_t, typename Cont_T<transaction>::const_iterator>
    search(Hash_T const& expected_payment_id,
           Cont_T<transaction> const& txs,
           bool skip_coinbase = true)
    {

        //auto null_hash = null_hash_T();

        uint64_t found_amount {0};

        Hash_T null_hash {};

        // iterator to last txs that we found containing
        // our payment
        typename Cont_T<transaction>::const_iterator found_tx_it
                = std::cend(txs);

        for (auto it = std::cbegin(txs);
             it != std::cend(txs); ++it)
        {
            auto const& tx = *it;

            bool is_coinbase_tx = is_coinbase(tx);

            if (skip_coinbase && is_coinbase_tx)
                continue;

            // get payment id. by default we are intrested
            // in short ids from integrated addresses
            auto payment_id_tuple = xmreg::get_payment_id(tx);

            auto pay_id = std::get<Hash_T>(payment_id_tuple);

            if (pay_id == null_hash)
                continue;

            public_key tx_pub_key
                    = xmreg::get_tx_pub_key_from_received_outs(tx);

            // we have pay_id. it can be crypto::hash8 or crypto:hash
            // so naw we need to perform comparison of the pay_id found
            // with the expected value of payment_id
            // for hash8 we need to do decoding of the pay_id, as it is
            // encoded

            if (!payment_id_decryptor(pay_id, tx_pub_key))
            {
                throw PaymentSearcherException("Cant decrypt pay_id: "
                                               + pod_to_hex(pay_id));
            }

            if (pay_id != expected_payment_id)
                continue;

            // if everything ok with payment id, we proceed with
            // checking how much xmr we got in that tx.

            // for each output, in a tx, check if it belongs
            // to the given account of specific address and viewkey

            auto tx_hash = get_transaction_hash(tx);

            OutputInputIdentification oi_identification {
                        &address_info, &view_key, &tx,
                        tx_hash, is_coinbase_tx};

            oi_identification.identify_outputs();


            if (!oi_identification.identified_outputs.empty())
            {   // nothing was found
                make_pair(found_amount, found_tx_it);
            }

            // now add the found outputs into Outputs tables
            for (auto& out_info: oi_identification.identified_outputs)
                found_amount += out_info.amount;

            if (found_amount > 0)
                found_tx_it = it;
        }

        return make_pair(found_amount, found_tx_it);
    }


    virtual ~PaymentSearcher() = default;

private:
    address_parse_info const& address_info;
    secret_key const& view_key;
    Hash_T payment_id;
    MicroCore* const mcore;


    inline bool
    payment_id_decryptor(
            crypto::hash& payment_id,
            public_key const& tx_pub_key)
    {
        // don't need to do anything for legacy payment ids
        return true;
    }

    inline bool
    payment_id_decryptor(
            crypto::hash8& payment_id,
            public_key const& tx_pub_key)
    {
        // overload for short payment id,
        // the we are going to decrypt
        return mcore->decrypt_payment_id(
                    payment_id, tx_pub_key, view_key);
    }

};



}

