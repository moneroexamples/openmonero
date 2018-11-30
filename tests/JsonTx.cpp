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

    addr_and_viewkey_from_string(
             jtx["sender"]["address"], jtx["sender"]["viewkey"],                                \
             ntype, sender.address, sender.viewkey);

    parse_str_secret_key(jtx["sender"]["spendkey"], sender.spendkey);

    sender.amount = jtx["sender"]["amount"];
    sender.change = jtx["sender"]["change"];

    for (auto const& recpient: jtx["recipient"])
    {
        recipients.push_back(account{});

        addr_and_viewkey_from_string(
                 recpient["address"], recpient["viewkey"],                                \
                 ntype, recipients.back().address,
                 recipients.back().viewkey);

        parse_str_secret_key(recpient["spendkey"],
                recipients.back().spendkey);

        recipients.back().amount = recpient["amount"];
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


}
