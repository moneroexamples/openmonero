#pragma once

#include "om_log.h"
#include "OutputInputIdentification.h"
#include "UniversalIdentifier.hpp"

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

public:
    PaymentSearcher(
            address_parse_info const& _address_info,
            secret_key const& _viewkey):
          address_info {_address_info},
          viewkey {_viewkey}
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
        auto found_tx_it = std::cend(txs);

        for (auto it = std::cbegin(txs);
             it != std::cend(txs); ++it)
        {
            auto const& tx = *it;

            bool is_coinbase_tx = is_coinbase(tx);

            if (skip_coinbase && is_coinbase_tx)
                continue;

            auto identifier = make_identifier(tx,
                  make_unique<Output>(&address_info, &viewkey),
                  make_unique<IntegratedPaymentID>(&address_info, &viewkey));


            auto tx_pub_key = identifier.get_tx_pub_key();

            identifier.template get<IntegratedPaymentID>()
                    ->identify(tx, tx_pub_key);

            auto pay_id = identifier.template get<IntegratedPaymentID>()
                    ->get_id();

            // we have pay_id. it can be crypto::hash8 or crypto:hash
            // so naw we need to perform comparison of the pay_id found
            // with the expected value of payment_id
            // for hash8 we need to do decoding of the pay_id, as it is
            // encoded

            if (pay_id != expected_payment_id)
                continue;

            // if everything ok with payment id, we proceed with
            // checking how much xmr we got in that tx.

            // for each output, in a tx, check if it belongs
            // to the given account of specific address and viewkey

            identifier.template get<Output>()->identify(tx, tx_pub_key);

            auto const& outputs_found
                    = identifier.template get<Output>()->get();

            if (!outputs_found.empty())
            {   // nothing was found
                make_pair(found_amount, found_tx_it);
            }

            // now add the found outputs into Outputs tables
            for (auto& out_info: outputs_found)
                found_amount += out_info.amount;

            if (found_amount > 0)
                found_tx_it = it;
        }

        return make_pair(found_amount, found_tx_it);
    }

    virtual ~PaymentSearcher() = default;

private:
    address_parse_info const& address_info;
    secret_key const& viewkey;

};



}

