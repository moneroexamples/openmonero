//
// Created by mwo on 13/02/17.
//

#include "OutputInputIdentification.h"


namespace xmreg
{

OutputInputIdentification::OutputInputIdentification(
    const address_parse_info* _a,
    const secret_key* _v,
    const transaction* _tx)
    : total_received {0}, mixin_no {0}
{
    address_info = _a;
    viewkey = _v;
    tx = _tx;

    tx_hash     = get_transaction_hash(*tx);
    tx_pub_key     = xmreg::get_tx_pub_key_from_received_outs(*tx);

    tx_is_coinbase = is_coinbase(*tx);

    is_rct = (tx->version == 2);

    if (is_rct)
    {
        rct_type = tx->rct_signatures.type;
    }


    if (!generate_key_derivation(tx_pub_key, *viewkey, derivation))
    {
        cerr << "Cant get derived key for: "  << "\n"
             << "pub_tx_key: " << get_tx_pub_key_str() << " and "
             << "prv_view_key" << viewkey << endl;

        throw OutputInputIdentificationException("Cant get derived key for a tx");
    }

    if (!tx_is_coinbase)
    {

    }

}

uint64_t
OutputInputIdentification::get_mixin_no()
{
    if (mixin_no == 0 && !tx_is_coinbase)
        mixin_no = xmreg::get_mixin_no(*tx);

    return mixin_no;
}

void
OutputInputIdentification::identify_outputs()
{
    //          <public_key  , amount  , out idx>
    vector<tuple<txout_to_key, uint64_t, uint64_t>> outputs;

    outputs = get_ouputs_tuple(*tx);


    for (auto& out: outputs)
    {
        txout_to_key txout_k      = std::get<0>(out);
        uint64_t amount           = std::get<1>(out);
        uint64_t output_idx_in_tx = std::get<2>(out);

        // get the tx output public key
        // that normally would be generated for us,
        // if someone had sent us some xmr.
        public_key generated_tx_pubkey;

        derive_public_key(derivation,
                          output_idx_in_tx,
                          address_info->address.m_spend_public_key,
                          generated_tx_pubkey);

        // check if generated public key matches the current output's key
        bool mine_output = (txout_k.key == generated_tx_pubkey);


        //cout  << "Chekcing output: "  << pod_to_hex(txout_k.key) << " "
        //      << "mine_output: " << mine_output << endl;


        // placeholder variable for ringct outputs info
        // that we need to save in database
        string rtc_outpk;
        string rtc_mask;
        string rtc_amount;

        // if mine output has RingCT, i.e., tx version is 2
        // need to decode its amount. otherwise its zero.
        if (mine_output && tx->version == 2)
        {
            // initialize with regular amount value
            // for ringct, except coinbase, it will be 0
            uint64_t rct_amount_val = amount;

            // cointbase txs have amounts in plain sight.
            // so use amount from ringct, only for non-coinbase txs
            if (!tx_is_coinbase)
            {
                bool r;

                // for ringct non-coinbase txs, these values are provided with txs.
                // coinbase ringctx dont have this information. we will provide
                // them only when needed, in get_unspent_outputs. So go there
                // to see how we deal with ringct coinbase txs when we spent them
                // go to CurrentBlockchainStatus::construct_output_rct_field
                // to see how we deal with coinbase ringct that are used as mixins
                rtc_outpk  = pod_to_hex(tx->rct_signatures.outPk[output_idx_in_tx].mask);
                rtc_mask   = pod_to_hex(tx->rct_signatures.ecdhInfo[output_idx_in_tx].mask);
                rtc_amount = pod_to_hex(tx->rct_signatures.ecdhInfo[output_idx_in_tx].amount);

                rct::key mask =  tx->rct_signatures.ecdhInfo[output_idx_in_tx].mask;

                r = decode_ringct(tx->rct_signatures,
                                  tx_pub_key,
                                  *viewkey,
                                  output_idx_in_tx,
                                  mask,
                                  rct_amount_val);

                if (!r)
                {
                    cerr << "Cant decode ringCT!" << endl;
                    throw OutputInputIdentificationException("Cant decode ringCT!");
                }

                amount = rct_amount_val;
            }

        } // if (mine_output && tx.version == 2)

        if (mine_output)
        {
            string out_key_str = pod_to_hex(txout_k.key);

            // found an output associated with the given address and viewkey
            string msg = fmt::format("tx_hash:  {:s}, output_pub_key: {:s}\n",
                                     get_tx_hash_str(),
                                     out_key_str);

            cout << msg << endl;


            total_received += amount;

            identified_outputs.emplace_back(
                    output_info{
                            out_key_str, amount, output_idx_in_tx,
                            rtc_outpk, rtc_mask, rtc_amount
                    });

        } //  if (mine_output)

    } // for (const auto& out: outputs)

}


void
OutputInputIdentification::identify_inputs(
        const vector<pair<string, uint64_t>>& known_outputs_keys)
{
    vector<txin_to_key> input_key_imgs = xmreg::get_key_images(*tx);

    // make timescale maps for mixins in input
    for (const txin_to_key& in_key: input_key_imgs)
    {
        // get absolute offsets of mixins
        std::vector<uint64_t> absolute_offsets
                = cryptonote::relative_output_offsets_to_absolute(
                        in_key.key_offsets);

        // get public keys of outputs used in the mixins that match to the offests
        std::vector<cryptonote::output_data_t> mixin_outputs;


        if (!CurrentBlockchainStatus::get_output_keys(in_key.amount,
                                                      absolute_offsets,
                                                      mixin_outputs))
        {
            cerr << "Mixins key images not found" << endl;
            continue;
        }


        // mixin counter
        size_t count = 0;

        // indicates whether we found any matching mixin in the current input
        bool found_a_match {false};

        // for each found output public key check if its ours or not
        for (const uint64_t& abs_offset: absolute_offsets)
        {
            // get basic information about mixn's output
            cryptonote::output_data_t output_data = mixin_outputs.at(count);

            string output_public_key_str = pod_to_hex(output_data.pubkey);

            //cout << " - output_public_key_str: " << output_public_key_str << endl;

            // before going to the mysql, check our known outputs cash
            // if the key exists. Its much faster than going to mysql
            // for this.


            auto it =  std::find_if(
                    known_outputs_keys.begin(),
                    known_outputs_keys.end(),
                    [&](const pair<string, uint64_t>& known_output)
                    {
                        return output_public_key_str == known_output.first;
                    });

            if (it == known_outputs_keys.end())
            {
                // this mixins's output is unknown.
                ++count;
                continue;
            }

            // this seems to be our mixin.
            // save it into identified_inputs vector

            identified_inputs.push_back(input_info {
                    pod_to_hex(in_key.k_image),
                    (*it).second, // amount
                    output_public_key_str});

            found_a_match = true;

            ++count;

        } // for (const cryptonote::output_data_t& output_data: outputs)

        if (found_a_match == false)
        {
            // if we didnt find any match, break of the look.
            // there is no reason to check remaining key images
            // as when we spent something, our outputs should be
            // in all inputs in a given txs. Thus, if a single input
            // is without our output, we can assume this tx does
            // not contain any of our spendings.
            break;
        }

    } // for (const txin_to_key& in_key: input_key_imgs)

}


string const&
OutputInputIdentification::get_tx_hash_str()
{
    if (tx_hash_str.empty())
    {
        tx_hash_str = pod_to_hex(tx_hash);
    }


    return tx_hash_str;
}


string const&
OutputInputIdentification::get_tx_prefix_hash_str()
{
    if (tx_prefix_hash_str.empty())
    {
        tx_prefix_hash = get_transaction_prefix_hash(*tx);
        tx_prefix_hash_str = pod_to_hex(tx_prefix_hash);
    }

    return tx_prefix_hash_str;
}

string const&
OutputInputIdentification::get_tx_pub_key_str()
{
    if (tx_pub_key_str.empty())
        tx_pub_key_str = pod_to_hex(tx_pub_key);

    return tx_pub_key_str;
}

}
