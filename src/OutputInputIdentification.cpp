//
// Created by mwo on 13/02/17.
//

#include "OutputInputIdentification.h"


namespace xmreg
{

OutputInputIdentification::OutputInputIdentification(
    const account_public_address* _a,
    const secret_key* _v,
    const transaction* _tx)
    : total_received {0}
{
    address = _a;
    viewkey = _v;
    tx = _tx;

    tx_hash        = get_transaction_hash(*tx);
    tx_prefix_hash = get_transaction_prefix_hash(*tx);
    tx_pub_key = xmreg::get_tx_pub_key_from_received_outs(*tx);

    tx_hash_str        = pod_to_hex(tx_hash);
    tx_prefix_hash_str = pod_to_hex(tx_prefix_hash);
    tx_pub_key_str     = pod_to_hex(tx_pub_key);

    tx_is_coinbase = is_coinbase(*tx);

    if (!generate_key_derivation(tx_pub_key, *viewkey, derivation))
    {
        cerr << "Cant get derived key for: "  << "\n"
             << "pub_tx_key: " << tx_pub_key << " and "
             << "prv_view_key" << viewkey << endl;

        throw OutputInputIdentificationException("Cant get derived key for a tx");
    }
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
                          address->m_spend_public_key,
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

            rtc_outpk  = pod_to_hex(tx->rct_signatures.outPk[output_idx_in_tx].mask);
            rtc_mask   = pod_to_hex(tx->rct_signatures.ecdhInfo[output_idx_in_tx].mask);
            rtc_amount = pod_to_hex(tx->rct_signatures.ecdhInfo[output_idx_in_tx].amount);

            // cointbase txs have amounts in plain sight.
            // so use amount from ringct, only for non-coinbase txs
            if (!tx_is_coinbase)
            {
                bool r;

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
                                     tx_hash_str,
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
        const vector<string>& known_outputs_keys)
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

        //cout << "in_key.k_image): " << pod_to_hex(in_key.k_image) << endl;


        // mixin counter
        size_t count = 0;

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

            if (std::find(
                    known_outputs_keys.begin(),
                    known_outputs_keys.end(),
                    output_public_key_str)
                == known_outputs_keys.end())
            {
                // this mixins's output is unknown.
                ++count;
                continue;
            }


            XmrOutput out;

            if (xmr_accounts->output_exists(output_public_key_str, out))
            {
                cout << "input uses some mixins which are our outputs"
                     << out << endl;

                // seems that this key image is ours.
                // so save it to database for later use.

                XmrInput in_data;

                in_data.account_id  = acc->id;
                in_data.tx_id       = 0; // for now zero, later we set it
                in_data.output_id   = out.id;
                in_data.key_image   = pod_to_hex(in_key.k_image);
                in_data.amount      = out.amount; // must match corresponding output's amount
                in_data.timestamp   = blk_timestamp_mysql_format;

                inputs_found.push_back(in_data);

                // a key image has only one real mixin. Rest is fake.
                // so if we find a candidate, break the search.

                // break;

            } // if (xmr_accounts->output_exists(output_public_key_str, out))

            count++;

        } // for (const cryptonote::output_data_t& output_data: outputs)

    } // for (const txin_to_key& in_key: input_key_imgs)



}

}