#include "RandomOutputs.h"
#include "CurrentBlockchainStatus.h"
namespace xmreg
{

RandomOutputs::RandomOutputs(
        CurrentBlockchainStatus const* _cbs,
        vector<uint64_t> const& _amounts,
        uint64_t _outs_count)
    : cbs {_cbs},
      amounts {_amounts},
      outs_count {_outs_count}
{
}

uint64_t
RandomOutputs::get_random_output_index(uint64_t num_outs) const
{
    // type = "triangular"; from wallet2.cpp
    static uint64_t const shift_factor {1ull << 53};

    uint64_t r = crypto::rand<uint64_t>() % shift_factor;
    double frac = std::sqrt(static_cast<double>(r) / shift_factor);
    uint64_t i = static_cast<uint64_t>(frac * num_outs);

    i = (i == num_outs ? --i : i);

    return i;
}

bool
RandomOutputs::get_output_pub_key(
        uint64_t amount,
        uint64_t global_output_index,
        crypto::public_key& out_pk) const
{
    COMMAND_RPC_GET_OUTPUTS_BIN::request req;
    COMMAND_RPC_GET_OUTPUTS_BIN::response res;

    req.outputs.push_back(get_outputs_out {amount, global_output_index});

    if (!cbs->get_outs(req, res))
    {
        OMERROR << "mcore->get_outs(req, res) failed";
        return false;
    }

    if (res.outs.empty())
        return false;

    out_pk = res.outs[0].key;

    return true;
}

bool
RandomOutputs::find_random_outputs()
{
    COMMAND_RPC_GET_OUTPUT_HISTOGRAM::request req;
    COMMAND_RPC_GET_OUTPUT_HISTOGRAM::response res;

    req.amounts = amounts;
    req.unlocked = true;
    req.recent_cutoff = 0;
    req.min_count = outs_count;
    req.max_count = 0;

    if (!cbs->get_output_histogram(req, res))
    {
        OMERROR << "mcore->get_output_histogram(req, res)";
        return false;
    }

    found_outputs.clear();

    using histogram_entry = COMMAND_RPC_GET_OUTPUT_HISTOGRAM::entry;

    for (uint64_t amount: amounts)
    {
        // find histogram_entry for amount that we look
        // random outputs for
        auto const hist_entry_it = std::find_if(
             std::begin(res.histogram), std::end(res.histogram),
             [&amount](auto const& he)
            {
                 return amount == he.amount;
            });

        if (hist_entry_it == res.histogram.end())
        {
            OMERROR << "Could not find amount: it "
                       "== res.histogram.end()\n";
            return false;
        }

        RandomOutputs::outs_for_amount outs_info;
        outs_info.amount = amount;

        uint64_t num_outs = hist_entry_it->unlocked_instances;

        // keep track of seen random_global_amount_idx
        // so that we don't use same idx twice
        std::unordered_set<uint64_t> seen_indices;

        // use it as a failself, as we don't want infinit loop here
        size_t trial_i {0};

        while (seen_indices.size() < outs_count)
        {
            if (trial_i++ > max_no_of_trials)
            {
                OMERROR << "Can't find random output: maximum number "
                           "of trials reached";
                return false;
            }

            uint64_t random_global_amount_idx
                    = get_random_output_index(num_outs);

            if (seen_indices.count(random_global_amount_idx) > 0)
                continue;

            seen_indices.emplace(random_global_amount_idx);

            crypto::public_key found_output_public_key;

            if (!get_output_pub_key(amount,
                                    random_global_amount_idx,
                                    found_output_public_key))
            {
                OMERROR << "Cant find outputs public key for amount "
                        << amount << " and global_index of "
                        << random_global_amount_idx;
            }

            outs_info.outs.push_back({random_global_amount_idx,
                                      found_output_public_key});
        }

        found_outputs.push_back(outs_info);
    }

    return true;
}
}
