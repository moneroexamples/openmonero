#include "JsonTx.h"

namespace xmreg
{

JsonTx::JsonTx(json _jtx): jtx {std::move(_jtx)}
{

}

JsonTx::JsonTx(string _path): jpath {std::move(_path)}
{
    if (!read_config())
    {
        throw std::runtime_error("Cant read " + jpath);
    }

    init();
}

void
JsonTx::init()
{
    ntype = cryptonote::network_type {jtx["nettype"]};

    hex_to_pod(jtx["payment_id"], payment_id);
    hex_to_pod(jtx["payment_id8"], payment_id8);
    hex_to_pod(jtx["payment_id8e"], payment_id8e);

    addr_and_viewkey_from_string(
             jtx["sender"]["address"], jtx["sender"]["viewkey"],                                \
             ntype, sender.address, sender.viewkey);

    parse_str_secret_key(jtx["sender"]["spendkey"], sender.spendkey);

    sender.amount = jtx["sender"]["amount"];
    sender.change = jtx["sender"]["change"];
    sender.ntype = ntype;

    populate_outputs(jtx["sender"]["outputs"], sender.outputs);

    for (auto const& jrecpient: jtx["recipient"])
    {
        recipients.push_back(account{});

        addr_and_viewkey_from_string(
                 jrecpient["address"], jrecpient["viewkey"],                                \
                 ntype, recipients.back().address,
                 recipients.back().viewkey);

        parse_str_secret_key(jrecpient["spendkey"],
                recipients.back().spendkey);

        recipients.back().amount = jrecpient["amount"];

        recipients.back().is_subaddress = jrecpient["is_subaddress"];
        recipients.back().ntype = ntype;

        populate_outputs(jrecpient["outputs"], recipients.back().outputs);
    }

    if (!hex_to_tx(jtx["hex"], tx, tx_hash, tx_prefix_hash))
    {
        throw std::runtime_error("hex_to_tx(jtx[\"hex\"], tx, tx_hash, tx_prefix_hash)");
    }

}


bool
JsonTx::read_config()
{
    if (!boost::filesystem::exists(jpath))
    {
        cerr << "Config file " << jpath << " does not exist\n";
        return false;
    }

    try
    {
        // try reading and parsing json config file provided
        std::ifstream i(jpath);
        i >> jtx;
    }
    catch (std::exception const& e)
    {
        cerr << "Cant parse json string as json: " << e.what() << endl;
        return false;
    }

    return true;
}


bool
check_and_adjust_path(string& in_path)
{
    if (!boost::filesystem::exists(in_path))
        in_path = "./tests/" + in_path;

    return boost::filesystem::exists(in_path);
}

boost::optional<JsonTx>
construct_jsontx(string tx_hash)
{
    string tx_path  = "./tx/" + tx_hash + ".json";

    if (!check_and_adjust_path(tx_path))
    {
        return {};
    }

    return JsonTx {tx_path};
}

void
JsonTx::populate_outputs(json const& joutputs, vector<output>& outs)
{
    for (auto const& jout: joutputs)
    {
        public_key out_pk;

        if (!hex_to_pod(jout[1], out_pk))
        {
            throw std::runtime_error("hex_to_pod(jout[1], out_pk)");
        }

        output out {jout[0], out_pk, jout[2]};

        outs.push_back(out);
    }
}

}
