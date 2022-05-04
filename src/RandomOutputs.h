#ifndef RANDOMOUTPUTS_H
#define RANDOMOUTPUTS_H

#include "om_log.h"
#include "src/MicroCore.h"


namespace xmreg
{

class CurrentBlockchainStatus;

/**
 * @brief Returns random ouputs for given amounts
 *
 * This is a replacement for a method that used to be
 * avaliable in the monero itself, but which was dropped.
 *
 * The aim is to get a random ouputs of given amount.
 *
 * This is needed for frontend when its constructs a tx
 *
 */
class RandomOutputs
{
public:

    // how many times wy tray to get uinique random output
    // before we give up
    size_t const max_no_of_trials {100};

    // the two structures are here to make get_random_outs
    // method work as before. Normally, the used to be defined
    // in monero, but due to recent changes in 2018 09,
    // they were removed. However, parts of openmonero
    // require them.
    struct out_entry
    {
      uint64_t global_amount_index;
      crypto::public_key out_key;
    };

    struct outs_for_amount
    {
      uint64_t amount;
      std::list<out_entry> outs;
    };

    using outs_for_amount_v = vector<outs_for_amount>;

    RandomOutputs(CurrentBlockchainStatus const* _cbs,
                  vector<uint64_t> const& _amounts,
                  uint64_t _outs_count);

    virtual bool
    find_random_outputs();

    outs_for_amount_v
    get_found_outputs() const { return found_outputs; }

    virtual ~RandomOutputs() = default;

protected:
    //MicroCore const& mcore;
    CurrentBlockchainStatus const* cbs;

    vector<uint64_t> amounts;
    uint64_t outs_count;

    outs_for_amount_v found_outputs;

    virtual bool
    get_outputs_per_second(std::vector<uint64_t> rct_offsets, double& outputs_per_second) const;

    virtual bool
    gamma_pick(std::vector<uint64_t> rct_offsets,
               double outputs_per_second,
               uint64_t& decoy_output_index) const;

    virtual uint64_t
    triangular_pick(uint64_t num_outs) const;

    virtual bool
    get_output_pub_key(uint64_t amount,
                       uint64_t global_output_index,
                       crypto::public_key& out_pk,
                       bool& unlocked) const;

    virtual bool
    get_output_histogram(const std::vector<uint64_t> pre_rct_output_amounts,
                         std::vector<COMMAND_RPC_GET_OUTPUT_HISTOGRAM::entry>& histogram) const;

};

}

#endif // RANDOMOUTPUTS_H
