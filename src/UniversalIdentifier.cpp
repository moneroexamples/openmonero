#include "UniversalIdentifier.hpp"

namespace xmreg
{

void
Output::identify(transaction const& tx,
                 public_key const& tx_pub_key)
{
    cout << "Outputs identificataion" << endl;


    bool tx_is_coinbase = is_coinbase(tx);

    key_derivation derivation;

    if (!generate_key_derivation(tx_pub_key,
                                 *get_viewkey(), derivation))
    {
        throw std::runtime_error("Cant get derived key for a tx");
    }

    for (uint64_t i = 0; i < tx.vout.size(); ++i)
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

        // check if generated public key matches the current output's key
        bool mine_output = (txout_key.key == generated_tx_pubkey);


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
                bool r;

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

                r = decode_ringct(tx.rct_signatures,
                                  tx_pub_key,
                                  *get_viewkey(),
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
                        txout_key.key, amount, i,
                        rtc_outpk, rtc_mask, rtc_amount
                    });

        } //  if (mine_output)

    } // for (uint64_t i = 0; i < tx.vout.size(); ++i)
}

void Input::identify(transaction const& tx,
              public_key const& tx_pub_key)
{
     cout << "Inputs identificataion" << endl;
}


}
