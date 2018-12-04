#include "UniversalIdentifier.hpp"

namespace xmreg
{

void
Output::identify(transaction const& tx,
                 public_key const& tx_pub_key,
                 vector<public_key> const& additional_tx_pub_keys)
{   
    auto tx_is_coinbase = is_coinbase(tx);

    key_derivation derivation;

    if (!generate_key_derivation(tx_pub_key,
                                 *get_viewkey(), derivation))
    {
        throw std::runtime_error("generate_key_derivation failed");
    }

    // since introduction of subaddresses, a tx can
    // have extra public keys, thus we need additional
    // derivations

    vector<key_derivation> additional_derivations;

    if (!additional_tx_pub_keys.empty())
    {
        additional_derivations.resize(additional_tx_pub_keys.size());

        for (size_t i = 0; i < additional_tx_pub_keys.size(); ++i)
        {
            if (!generate_key_derivation(additional_tx_pub_keys[i],
                                         *get_viewkey(),
                                         additional_derivations[i]))
            {
                throw std::runtime_error("additional generate_key_derivation failed");
            }
        }
    }

    for (auto i = 0u; i < tx.vout.size(); ++i)
    {
        // i will act as output indxes in the tx

        if (tx.vout[i].target.type() != typeid(txout_to_key))
            continue;

        // get tx input key
        txout_to_key const& txout_key
                = boost::get<txout_to_key>(tx.vout[i].target);

        uint64_t amount = tx.vout[i].amount;

        // get the tx output public key
        // that normally would be generated for us,
        // if someone had sent us some xmr.
        public_key generated_tx_pubkey;

        derive_public_key(derivation,
                          i,
                          get_address()->address.m_spend_public_key,
                          generated_tx_pubkey);

//        cout << pod_to_hex(derivation) << ", " << i << ", "
//             << pod_to_hex(get_address()->address.m_spend_public_key) << ", "
//             << pod_to_hex(txout_key.key) << " == "
//             << pod_to_hex(generated_tx_pubkey) << '\n'  << '\n';

        // check if generated public key matches the current output's key
        bool mine_output = (txout_key.key == generated_tx_pubkey);

        auto with_additional = false;

        if (!mine_output && !additional_tx_pub_keys.empty())
        {
            // check for output using additional tx public keys
            derive_public_key(additional_derivations[i],
                              i,
                              get_address()->address.m_spend_public_key,
                              generated_tx_pubkey);


            mine_output = (txout_key.key == generated_tx_pubkey);

            with_additional = true;
        }

        // placeholder variable for ringct outputs info
        // that we need to save in database

        rct::key rtc_outpk {0};
        rct::key rtc_mask {0};
        rct::key rtc_amount {0};

        // if mine output has RingCT, i.e., tx version is 2
        // need to decode its amount. otherwise its zero.
        if (mine_output && tx.version == 2)
        {
            // initialize with regular amount value
            // for ringct, except coinbase, it will be 0
            uint64_t rct_amount_val = amount;

            // cointbase txs have amounts in plain sight.
            // so use amount from ringct, only for non-coinbase txs
            if (!tx_is_coinbase)
            {
                // for ringct non-coinbase txs, these values are given
                // with txs.
                // coinbase ringctx dont have this information. we will provide
                // them only when needed, in get_unspent_outputs. So go there
                // to see how we deal with ringct coinbase txs when we spent
                // them
                // go to CurrentBlockchainStatus::construct_output_rct_field
                // to see how we deal with coinbase ringct that are used
                // as mixins

                rtc_outpk = tx.rct_signatures.outPk[i].mask;
                rtc_mask = tx.rct_signatures.ecdhInfo[i].mask;
                rtc_amount = tx.rct_signatures.ecdhInfo[i].amount;

                rct::key mask =  tx.rct_signatures
                        .ecdhInfo[i].mask;

                auto r = decode_ringct(tx.rct_signatures,
                                       !with_additional ? derivation
                                             : additional_derivations[i],
                                       i,
                                       mask,
                                       rct_amount_val);

                if (!r)
                {
                    throw std::runtime_error(
                                "Cant decode ringCT!");
                }

                amount = rct_amount_val;

            } // if (!tx_is_coinbase)

        } // if (mine_output && tx.version == 2)

        if (mine_output)
        {
            total_received += amount;

            identified_outputs.emplace_back(
                    info{
                        txout_key.key, amount, i, derivation,
                        rtc_outpk, rtc_mask, rtc_amount
                    });

            total_xmr += amount;

        } //  if (mine_output)

    } // for (uint64_t i = 0; i < tx.vout.size(); ++i)
}

void Input::identify(transaction const& tx,
                     public_key const& tx_pub_key,
                     vector<public_key> const& additional_tx_pub_keys)
{
     auto search_misses {0};

     auto input_no = tx.vin.size();

     for (auto i = 0u; i < input_no; ++i)
     {
         if(tx.vin[i].type() != typeid(txin_to_key))
             continue;

         // get tx input key
         txin_to_key const& in_key
                 = boost::get<txin_to_key>(tx.vin[i]);

         // get absolute offsets of mixins
         vector<uint64_t> absolute_offsets
                 = relative_output_offsets_to_absolute(
                         in_key.key_offsets);

         // get public keys of outputs used in the mixins that
         // match to the offests
         vector<output_data_t> mixin_outputs;

         // this can THROW if no outputs are found
         mcore->get_output_key(in_key.amount,
                               absolute_offsets,
                               mixin_outputs);

         // indicates whether we found any matching mixin in the current input
         auto found_a_match {false};

         // for each found output public key check if its ours or not
         for (auto count = 0u; count < absolute_offsets.size(); ++count)
         {
             // get basic information about mixn's output
             output_data_t const& output_data
                     = mixin_outputs.at(count);

             // before going to the mysql, check our known outputs cash
             // if the key exists. Its much faster than going to mysql
             // for this.

             auto it = known_outputs->find(output_data.pubkey);

             if (it != known_outputs->end())
             {
                 // this seems to be our mixin.
                 // save it into identified_inputs vector

                 identified_inputs.push_back(info {
                         in_key.k_image,
                         it->second, // amount
                         output_data.pubkey});

                 total_xmr += it->second;

                 found_a_match = true;
             }

         } // for (const cryptonote::output_data_t& output_data: outputs)

         if (found_a_match == false)
         {
             // if we didnt find any match, break of the look.
             // there is no reason to check remaining key images
             // as when we spent something, our outputs should be
             // in all inputs in a given txs. Thus, if a single input
             // is without our output, we can assume this tx does
             // not contain any of our spendings.

             // just to be sure before we break out of this loop,
             // do it only after two misses

             if (++search_misses > 2)
                 break;
         }

     } //  for (auto i = 0u; i < input_no; ++i)
}


void RealInput::identify(transaction const& tx,
                         public_key const& tx_pub_key,
                         vector<public_key> const& additional_tx_pub_keys)
{
     auto search_misses {0};

     auto input_no = tx.vin.size();

     for (auto i = 0u; i < input_no; ++i)
     {
         if(tx.vin[i].type() != typeid(txin_to_key))
             continue;

         // get tx input key
         txin_to_key const& in_key
                 = boost::get<cryptonote::txin_to_key>(tx.vin[i]);

         // get absolute offsets of mixins
         auto absolute_offsets
                 = relative_output_offsets_to_absolute(
                         in_key.key_offsets);


         //tx_out_index is pair::<transaction hash, output index>
         vector<tx_out_index> indices;

         // get tx hashes and indices in the txs for the
         // given outputs of mixins
         //  this cant THROW DB_EXCEPTION
         mcore->get_output_tx_and_index(
                     in_key.amount, absolute_offsets, indices);

         // placeholder for information about key image that
         // we will find as ours
         unique_ptr<info> key_image_info {nullptr};

         // for each found mixin tx, check if any key image
         // generated using our outputs in the mixin tx
         // matches the given key image in the current tx
         for (auto const& txi : indices)
         {
            auto const& mixin_tx_hash = txi.first;
            auto const& output_index_in_tx = txi.second;

            transaction mixin_tx;

            if (!mcore->get_tx(mixin_tx_hash, mixin_tx))
            {
                throw std::runtime_error("Cant get tx: "
                                         + pod_to_hex(mixin_tx_hash));
            }

            // use Output universal identifier to identify our outputs
            // in a mixin tx
            auto identifier = make_identifier(
                        mixin_tx,
                        make_unique<Output>(get_address(), get_viewkey()));

            identifier.identify();

            for (auto const& found_output: identifier.get<Output>()->get())
            {
                // generate key_image using this output
                // to check for sure if the given key image is ours
                // or not
                crypto::key_image key_img_generated;

                if (!xmreg::generate_key_image(found_output.derivation,
                                               found_output.idx_in_tx, /* position in the tx */
                                               *spendkey,
                                               get_address()->address.m_spend_public_key,
                                               key_img_generated))
                {
                    throw std::runtime_error("Cant generate key image for output: "
                                                        + pod_to_hex(found_output.pub_key));
                }

                // now check if current key image in the tx which we
                // analyze matches generated key image
                if (in_key.k_image == key_img_generated)
                {
                    // this is our key image if they are equal!
                    key_image_info.reset(new info {key_img_generated,
                                                   found_output.amount,
                                                   found_output.pub_key});

                    break;
                }

            } // auto const& found_output: identifier.get<Output>()->get_outputs()

            // if key_image_info has been populated, there is no
            // reason to keep check remaning outputs in the mixin tx
            // instead add its info to identified_inputs and move on
            // to the next key image
            if (!key_image_info)
            {
                identified_inputs.push_back(*key_image_info);
                total_xmr += key_image_info->amount;
                break;
            }

         } // for (auto const& txi : indices)

     } //  for (auto i = 0u; i < input_no; ++i)
}

}
