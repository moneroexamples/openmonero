#include "RandomOutputs.h"
#include "CurrentBlockchainStatus.h"
namespace xmreg
{

constexpr const double GAMMA_SHAPE = 19.28;
constexpr const double GAMMA_SCALE = 1 / double(1.61);
std::gamma_distribution<double> gamma(GAMMA_SHAPE, GAMMA_SCALE);

constexpr const std::size_t BLOCKS_IN_A_YEAR = (86400 * 365) / DIFFICULTY_TARGET_V2;
constexpr const std::size_t DEFAULT_UNLOCK_TIME = CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE * DIFFICULTY_TARGET_V2;
constexpr const std::size_t RECENT_SPEND_WINDOW = 15 * DIFFICULTY_TARGET_V2;

RandomOutputs::RandomOutputs(
        CurrentBlockchainStatus const* _cbs,
        vector<uint64_t> const& _amounts,
        uint64_t _outs_count)
    : cbs {_cbs},
      amounts {_amounts},
      outs_count {_outs_count}
{
}

// copied a lot from monero-lws
bool
RandomOutputs::get_outputs_per_second(
        std::vector<uint64_t> rct_offsets,
        double &outputs_per_second) const
{
    if (CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE >= rct_offsets.size())
    {
        OMERROR << "Cannot get random outputs - blockchain height too small";
        return false;
    }

    const std::size_t blocks_to_consider = std::min(rct_offsets.size(), BLOCKS_IN_A_YEAR);
    const std::uint64_t initial = blocks_to_consider < rct_offsets.size() ?
        rct_offsets[rct_offsets.size() - blocks_to_consider - 1] : 0;
    const std::size_t outputs_to_consider = rct_offsets.back() - initial;
    static_assert(0 < DIFFICULTY_TARGET_V2, "block target time cannot be zero");
    // this assumes constant target over the whole rct range
    outputs_per_second = outputs_to_consider / double(DIFFICULTY_TARGET_V2 * blocks_to_consider);
    return true;
}

bool
RandomOutputs::gamma_pick(
        std::vector<uint64_t> rct_offsets, 
        double outputs_per_second,
        uint64_t& decoy_output_index) const
{
    static_assert(std::is_empty<crypto::random_device>(), 
                  "random_device is no longer cheap to construct");
    static constexpr const crypto::random_device engine{};
    const auto end = rct_offsets.end() - CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE;
    const uint64_t num_rct_outputs = *(rct_offsets.end() - CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE - 1);

    for (unsigned tries = 0; tries < max_no_of_trials; ++tries)
    {
        double output_age_in_seconds = std::exp(gamma(engine));

        // shift output back by unlock time to apply distribution from chain tip
        if (output_age_in_seconds > DEFAULT_UNLOCK_TIME)
            output_age_in_seconds -= DEFAULT_UNLOCK_TIME;
        else
            output_age_in_seconds = crypto::rand_idx(RECENT_SPEND_WINDOW);

        uint64_t output_index = output_age_in_seconds * outputs_per_second;
        if (num_rct_outputs <= output_index)
            continue; // gamma selected older than blockchain height (rare)

        output_index = num_rct_outputs - 1 - output_index;
        const auto selection = std::lower_bound(rct_offsets.begin(), end, output_index);
        if (selection == end)
        {
            OMERROR << "Unable to select random output -"
                       "output not found in range (should never happen)";
            return false;
        }

        const uint64_t first_rct = rct_offsets.begin() == selection ? 0 : *(selection - 1);
        const uint64_t n_rct = *selection - first_rct;
        if (n_rct != 0)
        {
            decoy_output_index = first_rct + crypto::rand_idx(n_rct);
            return true;
        }
        // block had zero outputs (miner didn't collect XMR?)
    }

    OMERROR << "Unable to select random output in spendable range "
               "using gamma distribution after 100 attempts";
    return false;
}

bool
RandomOutputs::get_output_histogram(
        const std::vector<uint64_t> pre_rct_output_amounts,
        std::vector<COMMAND_RPC_GET_OUTPUT_HISTOGRAM::entry> &histogram) const
{
    COMMAND_RPC_GET_OUTPUT_HISTOGRAM::request req;
    COMMAND_RPC_GET_OUTPUT_HISTOGRAM::response res;

    req.amounts = pre_rct_output_amounts;
    req.unlocked = true;
    req.recent_cutoff = 0;
    req.min_count = outs_count;
    req.max_count = 0;

    if (!cbs->get_output_histogram(req, res))
    {
        OMERROR << "mcore->get_output_histogram(req, res)";
        return false;
    }

    histogram = res.histogram;

    return true;
}

uint64_t
RandomOutputs::triangular_pick(uint64_t num_outs) const
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
        crypto::public_key& out_pk,
        bool& unlocked) const
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
    unlocked = res.outs[0].unlocked;

    return true;
}

bool
RandomOutputs::find_random_outputs()
{
    // seperate pre-RingCT requests from RingCT requests
    std::vector<uint64_t> pre_rct_output_amounts;
    bool request_rct_outputs = false;

    for (uint64_t amount: amounts)
    {
        if (amount != 0)
            pre_rct_output_amounts.push_back(amount);
        else
            request_rct_outputs = true;
    }

    // get the output histogram for selecting pre-RingCT decoys
    std::vector<COMMAND_RPC_GET_OUTPUT_HISTOGRAM::entry> histogram;
    if (!pre_rct_output_amounts.empty() 
        && !get_output_histogram(pre_rct_output_amounts, histogram))
        return false;

    // get the output distribution for selecting RingCT decoys
    std::vector<uint64_t> rct_offsets;
    double outputs_per_second;
    if (request_rct_outputs 
        && (!cbs->get_rct_output_distribution(rct_offsets)
            || !get_outputs_per_second(rct_offsets, outputs_per_second)))
        return false;

    found_outputs.clear();

    using histogram_entry = COMMAND_RPC_GET_OUTPUT_HISTOGRAM::entry;

    for (uint64_t amount: amounts)
    {
        RandomOutputs::outs_for_amount outs_info;
        outs_info.amount = amount;

        uint64_t num_pre_rct_outs;
        if (amount != 0)
        {
            // find histogram_entry for amount that we look
            // random outputs for
            auto const hist_entry_it = std::find_if(
                std::begin(histogram), std::end(histogram),
                [&amount](auto const& he)
            {
                return amount == he.amount;
            });

            if (hist_entry_it == histogram.end())
            {
                OMERROR << "Could not find amount: it "
                            "== histogram.end()\n";
                return false;
            }

            num_pre_rct_outs = hist_entry_it->unlocked_instances;
        }

        // keep track of seen random_global_amount_idx
        // so that we don't use same idx twice
        std::unordered_set<uint64_t> seen_indices;

        // use it as a failself, as we don't want infinit loop here
        size_t trial_i {0};

        while (outs_info.outs.size() < outs_count)
        {
            if (trial_i++ > max_no_of_trials)
            {
                OMERROR << "Can't find random output: maximum number "
                           "of trials reached";
                return false;
            }

            uint64_t random_global_amount_idx;
            if (amount != 0)
                random_global_amount_idx = triangular_pick(num_pre_rct_outs);
            else if (!gamma_pick(rct_offsets, outputs_per_second, random_global_amount_idx))
                return false;

            if (seen_indices.count(random_global_amount_idx) > 0)
                continue;

            seen_indices.emplace(random_global_amount_idx);

            crypto::public_key found_output_public_key;
            bool unlocked;

            if (!get_output_pub_key(amount,
                                    random_global_amount_idx,
                                    found_output_public_key,
                                    unlocked))
            {
                OMERROR << "Cant find outputs public key for amount "
                        << amount << " and global_index of "
                        << random_global_amount_idx;
                continue;
            }

            if (unlocked)
            {
                outs_info.outs.push_back({random_global_amount_idx,
                                          found_output_public_key});
            }
        }

        found_outputs.push_back(outs_info);
    }

    return true;
}
}
