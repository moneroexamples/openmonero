#pragma once

#include "om_log.h"
#include "tools.h"
#include "MicroCore.h"

#include <tuple>
#include <utility>

namespace xmreg
{
using namespace std;


class AbstractIdentifier
{
public:
    virtual void identify(transaction const& tx,
                          public_key const& tx_pub_key,
                          vector<public_key> const& additional_tx_pub_keys
                                = vector<public_key>{}) = 0;
};


class BaseIdentifier : public AbstractIdentifier
{
public:
    BaseIdentifier(
            address_parse_info const* _address,
            secret_key const* _viewkey)
        : address_info {_address}, viewkey {_viewkey}
   {}

    virtual void identify(transaction const& tx,
                          public_key const& tx_pub_key,
                          vector<public_key> const& additional_tx_pub_keys
                                = vector<public_key>{}) = 0;

    inline auto get_address() const {return address_info;}
    inline auto get_viewkey() const {return viewkey;}
    inline auto get_total() const {return total_xmr;}

    virtual ~BaseIdentifier() = default;

protected:
    address_parse_info const* address_info {nullptr};
    secret_key const* viewkey {nullptr};
    uint64_t total_xmr {0};
};

/**
 * @brief The Output class identifies our
 * outputs in a given tx
 */
class Output : public BaseIdentifier
{
public:

    Output(address_parse_info const* _address,
           secret_key const* _viewkey)
        : BaseIdentifier(_address, _viewkey)
    {}

    void identify(transaction const& tx,
                  public_key const& tx_pub_key,
                  vector<public_key> const& additional_tx_pub_keys
                        = vector<public_key>{}) override;

    inline auto get() const
    {
        return identified_outputs;
    }

    struct info
    {
        public_key pub_key;
        uint64_t   amount;
        uint64_t   idx_in_tx;
        key_derivation derivation;
        rct::key   rtc_outpk;
        rct::key   rtc_mask;
        rct::key   rtc_amount;

        friend std::ostream& operator<<(std::ostream& os, info const& _info);
    };

protected:

    uint64_t total_received {0};
    vector<info> identified_outputs;
};

/**
 * @brief The Input class identifies our possible
 * inputs (key images) in a given tx
 */
class Input : public BaseIdentifier
{
public:
                                        //output_pubk   , amount
    using known_outputs_t = unordered_map<public_key, uint64_t>;

    Input(address_parse_info const* _a,
           secret_key const* _viewkey,
           known_outputs_t const* _known_outputs,
           MicroCore* _mcore)
        : BaseIdentifier(_a, _viewkey),          
          known_outputs {_known_outputs},
          mcore {_mcore}
    {}

    void identify(transaction const& tx,
                  public_key const& tx_pub_key,
                  vector<public_key> const& additional_tx_pub_keys
                        = vector<public_key>{}) override;

    inline auto get() const
    {
        return identified_inputs;
    }


    struct info
    {
        key_image key_img;
        uint64_t amount;
        public_key out_pub_key;
    };


protected:

    secret_key const* viewkey {nullptr};   
    known_outputs_t const* known_outputs {nullptr};
    MicroCore* mcore {nullptr};
    vector<info> identified_inputs;
};

/**
 * Spendkey is optional. But if we have it,
 * we can for sure determine which key images
 * are ours or not. This is especially useful
 * in unit testing, since we can compare wether
 * guessed key images do contain all our key images
 */
class RealInput : public Input
{

public:

    RealInput(address_parse_info const* _a,
              secret_key const* _viewkey,
              secret_key const* _spendkey,
              MicroCore* _mcore)
        : Input(_a, _viewkey, nullptr, _mcore),
          spendkey {_spendkey}
    {}

    void identify(transaction const& tx,
                  public_key const& tx_pub_key,
                  vector<public_key> const& additional_tx_pub_keys
                        = vector<public_key>{}) override;


protected:
    secret_key const* spendkey {nullptr};
};


template <typename HashT>
class PaymentID : public BaseIdentifier
{

public:

    PaymentID(address_parse_info const* _address,
              secret_key const* _viewkey)
        : BaseIdentifier(_address, _viewkey)
    {}

    void identify(transaction const& tx,
                  public_key const& tx_pub_key,
                  vector<public_key> const& additional_tx_pub_keys
                        = vector<public_key>{}) override
    {   
        cout << "PaymentID decryption: "
                + pod_to_hex(payment_id) << endl;

        // get payment id. by default we are intrested
        // in short ids from integrated addresses
        auto payment_id_tuple = xmreg::get_payment_id(tx);

        payment_id = std::get<HashT>(payment_id_tuple);

        // if no payment_id found, return
        if (payment_id == null_hash)
            return;

        // decrypt integrated payment id. if its legacy payment id
        // nothing will happen.
        if (!decrypt(payment_id, tx_pub_key))
        {
            throw std::runtime_error("Cant decrypt pay_id: "
                                     + pod_to_hex(payment_id));
        }                
    }

    inline bool
    decrypt(crypto::hash& p_id,
            public_key const& tx_pub_key) const
    {
        // don't need to do anything for legacy payment ids
        return true;
    }

    inline bool
    decrypt(crypto::hash8& p_id,
            public_key const& tx_pub_key) const
    {
        // overload for short payment id,
        // the we are going to decrypt
        return xmreg::encrypt_payment_id(
                      p_id, tx_pub_key, *get_viewkey());
    }

    inline auto get() const {return payment_id;}

private:
    HashT payment_id {};
    HashT null_hash {};
};

using LegacyPaymentID = PaymentID<crypto::hash>;
using IntegratedPaymentID = PaymentID<crypto::hash8>;


template<typename... T>
class ModularIdentifier
{
public:
    tuple<unique_ptr<T>...> identifiers;

    ModularIdentifier(transaction const& _tx,
                      unique_ptr<T>... args)
        : identifiers {move(args)...},
          tx {_tx}
    {
        // having tx public key is very common for all identifiers
        // so we can get it here, instead of just obtaining it
        // for each identifier seprately
        tx_pub_key = get_tx_pub_key_from_received_outs(tx);

        // multi-output txs can have some additional public keys
        // in the extra field. So we also get them, just in case
        additional_tx_pub_keys = get_additional_tx_pub_keys_from_extra(tx);
    }

    void identify()
    {
         auto b = {(std::get<unique_ptr<T>>(
                        identifiers)->identify(
                            tx, tx_pub_key, additional_tx_pub_keys),
                   true)...};
         (void) b;
    }

    template <typename U>
    U* const get()
    {
        return std::get<unique_ptr<U>>(identifiers).get();
    }

    inline auto get_tx_pub_key() const {return tx_pub_key;}

private:
    transaction const& tx;
    public_key tx_pub_key;
    vector<public_key> additional_tx_pub_keys;
};

/**
 * A helper function to create ModularIdentifier object
 */
template<typename... T>
auto make_identifier(transaction const& tx, T&&... identifiers)
{
    return ModularIdentifier<typename T::element_type...>(
                tx, std::forward<T>(identifiers)...);
}

template <typename T>
auto
calc_total_xmr(T&& infos)
{
    uint64_t total_xmr {0};

    for (auto const& info: infos)
        total_xmr += info.amount;

    return total_xmr;
}


inline std::ostream&
operator<<(std::ostream& os, xmreg::Output::info const& _info)
{
    return os << _info.idx_in_tx << ", "
              << pod_to_hex(_info.pub_key) << ", "
              << _info.amount;
}

}

