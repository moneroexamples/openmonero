#ifndef RANDOMOUTPUTS_H
#define RANDOMOUTPUTS_H

#include "om_log.h"
#include "MicroCore.h"


namespace xmreg
{


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

    RandomOutputs(MicroCore const& _mcore,
                  vector<uint64_t> const& _amounts,
                  uint64_t _outs_count);

    virtual bool
    find_random_outputs();

    outs_for_amount_v
    get_found_outputs() const { return found_outputs; }

    virtual ~RandomOutputs() = default;

protected:
    MicroCore const& mcore;

    vector<uint64_t> amounts;
    uint64_t outs_count;

    outs_for_amount_v found_outputs;

    virtual uint64_t
    get_random_output_index(uint64_t num_outs) const;

    virtual bool
    get_output_pub_key(uint64_t amount,
                       uint64_t global_output_index,
                       crypto::public_key& out_pk) const;

};

}

#endif // RANDOMOUTPUTS_H
