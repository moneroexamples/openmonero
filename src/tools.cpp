//
// Created by marcin on 5/11/15.
//

#include "tools.h"
#include <codecvt>


namespace xmreg
{

/**
 * Parse key string, e.g., a viewkey in a string
 * into crypto::secret_key or crypto::public_key
 * depending on the template argument.
 */
template <typename T>
bool
parse_str_secret_key(const string& key_str, T& secret_key)
{

    // hash and keys have same structure, so to parse string of
    // a key, e.g., a view key, we can first parse it into the hash
    // object using parse_hash256 function, and then copy the reslting
    // hash data into secret key.
    crypto::hash hash_;

    if(!parse_hash256(key_str, hash_))
    {
        cerr << "Cant parse a key (e.g. viewkey): " << key_str << endl;
        return false;
    }

    // crypto::hash and crypto::secret_key have basicly same
    // structure. They both keep they key/hash as c-style char array
    // of fixed size. Thus we can just copy data from hash
    // to key
    copy(begin(hash_.data), end(hash_.data), secret_key.data);

    return true;
}

// explicit instantiations of get template function
template bool parse_str_secret_key<crypto::secret_key>(const string& key_str, crypto::secret_key& secret_key);
template bool parse_str_secret_key<crypto::public_key>(const string& key_str, crypto::public_key& secret_key);
template bool parse_str_secret_key<crypto::hash>(const string& key_str, crypto::hash& secret_key);

/**
 * Get transaction tx using given tx hash. Hash is represent as string here,
 * so before we can tap into the blockchain, we need to pare it into
 * crypto::hash object.
 */
bool
get_tx_pub_key_from_str_hash(Blockchain& core_storage, const string& hash_str, transaction& tx)
{
    crypto::hash tx_hash;
    parse_hash256(hash_str, tx_hash);

    try
    {
        // get transaction with given hash
        tx = core_storage.get_db().get_tx(tx_hash);
    }
    catch (const TX_DNE& e)
    {
        cerr << e.what() << endl;
        return false;
    }

    return true;
}

/**
 * Parse monero address in a string form into
 * cryptonote::account_public_address object
 */

bool
parse_str_address(const string& address_str,
                  address_parse_info& address_info,
                  network_type net_type)
{

    if (!get_account_address_from_str(address_info, net_type, address_str))
    {
        cerr << "Error getting address: " << address_str << '\n';
        return false;
    }

    return true;
}


string
print_sig (const signature& sig)
{
    stringstream ss;

    ss << "c: <" << epee::string_tools::pod_to_hex(sig.c) << "> "
       << "r: <" << epee::string_tools::pod_to_hex(sig.r) << ">";

    return ss.str();
}

/**
 * Check if a character is a path seprator
 */
inline bool
is_separator(char c)
{
    // default linux path separator
    const char separator = PATH_SEPARARTOR;

    return c == separator;
}



/**
 * Remove trailinig path separator.
 */
string
remove_trailing_path_separator(const string& in_path)
{
    string new_string = in_path;
    if (!new_string.empty() && is_separator(new_string[new_string.size() - 1]))
        new_string.erase(new_string.size() - 1);
    return new_string;
}

bf::path
remove_trailing_path_separator(const bf::path& in_path)
{
    string path_str = in_path.native();
    return bf::path(remove_trailing_path_separator(path_str));
}


string
timestamp_to_str_gm(time_t timestamp, const char* format)
{
    const time_t* t = &timestamp;

    const int TIME_LENGTH = 60;

    char str_buff[TIME_LENGTH];

    std::tm tmp;
    gmtime_r(t, &tmp);

    size_t len;

    len = std::strftime(str_buff, TIME_LENGTH, format, &tmp);

    return string(str_buff, len);
}

string
timestamp_to_str_local(time_t timestamp, const char* format)
{

    const int TIME_LENGTH = 60;

    char str_buff[TIME_LENGTH];

    tm *tm_ptr;
    tm_ptr = localtime(&timestamp);

    size_t len;

    len = std::strftime(str_buff, TIME_LENGTH, format, tm_ptr);

    return string(str_buff, len);
}


ostream&
operator<< (ostream& os, const address_parse_info& addr_info)
{
    os << get_account_address_as_str(network_type::MAINNET,
                                     addr_info.is_subaddress,
                                     addr_info.address);
    return os;
}


/*
 * Generate key_image of foran ith output
 */
bool
generate_key_image(const crypto::key_derivation& derivation,
                   const std::size_t i,
                   const crypto::secret_key& sec_key,
                   const crypto::public_key& pub_key,
                   crypto::key_image& key_img)
{

    cryptonote::keypair in_ephemeral;

    if (!crypto::derive_public_key(derivation, i,
                                   pub_key,
                                   in_ephemeral.pub))
    {
        cerr << "Error generating publick key " << pub_key << endl;
        return false;
    }

    try
    {

        crypto::derive_secret_key(derivation, i,
                                  sec_key,
                                  in_ephemeral.sec);
    }
    catch(const std::exception& e)
    {
        cerr << "Error generate secret image: " << e.what() << endl;
        return false;
    }


    try
    {
        crypto::generate_key_image(in_ephemeral.pub,
                                   in_ephemeral.sec,
                                   key_img);
    }
    catch(const std::exception& e)
    {
        cerr << "Error generate key image: " << e.what() << endl;
        return false;
    }

    return true;
}


string
get_default_lmdb_folder(network_type nettype)
{
    // default path to monero folder
    // on linux this is /home/<username>/.bitmonero
    string default_monero_dir = tools::get_default_data_dir();

    if (nettype == cryptonote::network_type::TESTNET)
        default_monero_dir += "/testnet";
    if (nettype == cryptonote::network_type::STAGENET)
        default_monero_dir += "/stagenet";


    // the default folder of the lmdb blockchain database
    // is therefore as follows
    return default_monero_dir + string("/lmdb");
}



/*
 * Ge blockchain exception from command line option
 *
 * If not given, provide default path
 */
bool
get_blockchain_path(bf::path& blockchain_path,
                    cryptonote::network_type nettype)
{
    // the default folder of the lmdb blockchain database
    string default_lmdb_dir   = xmreg::get_default_lmdb_folder(nettype);

    blockchain_path = !blockchain_path.empty()
                      ? blockchain_path
                      : bf::path(default_lmdb_dir);

    if (!bf::is_directory(blockchain_path))
    {
        cerr << "Given path \"" << blockchain_path   << "\" "
             << "is not a folder or does not exist \n";

        return false;
    }

    blockchain_path = xmreg::remove_trailing_path_separator(blockchain_path);

    return true;
}

bool
get_blockchain_path(string& blockchain_path,
                    cryptonote::network_type nettype)
{
    bf::path p {blockchain_path};

    if (!get_blockchain_path(p, nettype))
        return false;

    blockchain_path = p.string();

    return true;
}


array<uint64_t, 4>
summary_of_in_out_rct(
        const transaction& tx,
        vector<pair<txout_to_key, uint64_t>>& output_pub_keys,
        vector<txin_to_key>& input_key_imgs)
{

    uint64_t xmr_outputs       {0};
    uint64_t xmr_inputs        {0};
    uint64_t mixin_no          {0};
    uint64_t num_nonrct_inputs {0};


    for (const tx_out& txout: tx.vout)
    {
        if (txout.target.type() != typeid(txout_to_key))
        {
            // push empty pair.
            output_pub_keys.push_back(pair<txout_to_key, uint64_t>{});
            continue;
        }

        // get tx input key
        const txout_to_key& txout_key
                = boost::get<cryptonote::txout_to_key>(txout.target);

        output_pub_keys.push_back(make_pair(txout_key, txout.amount));

        xmr_outputs += txout.amount;
    }

    size_t input_no = tx.vin.size();

    for (size_t i = 0; i < input_no; ++i)
    {

        if(tx.vin[i].type() != typeid(cryptonote::txin_to_key))
        {
            continue;
        }

        // get tx input key
        const cryptonote::txin_to_key& tx_in_to_key
                = boost::get<cryptonote::txin_to_key>(tx.vin[i]);

        xmr_inputs += tx_in_to_key.amount;

        if (tx_in_to_key.amount != 0)
        {
            ++num_nonrct_inputs;
        }

        if (mixin_no == 0)
        {
            mixin_no = tx_in_to_key.key_offsets.size();
        }

        input_key_imgs.push_back(tx_in_to_key);

    } //  for (size_t i = 0; i < input_no; ++i)


    return {xmr_outputs, xmr_inputs, mixin_no, num_nonrct_inputs};
};


// this version for mempool txs from json
array<uint64_t, 6>
summary_of_in_out_rct(const json& _json)
{
    uint64_t xmr_outputs       {0};
    uint64_t xmr_inputs        {0};
    uint64_t no_outputs        {0};
    uint64_t no_inputs         {0};
    uint64_t mixin_no          {0};
    uint64_t num_nonrct_inputs {0};

    for (const json& vout: _json["vout"])
    {
        xmr_outputs += vout["amount"].get<uint64_t>();
    }

    no_outputs = _json["vout"].size();

    for (const json& vin: _json["vin"])
    {
        uint64_t amount = vin["key"]["amount"].get<uint64_t>();

        xmr_inputs += amount;

        if (amount != 0)
            ++num_nonrct_inputs;
    }

    no_inputs  = _json["vin"].size();

    mixin_no = _json["vin"].at(0)["key"]["key_offsets"].size() - 1;

    return {xmr_outputs, xmr_inputs, no_outputs, no_inputs, mixin_no, num_nonrct_inputs};
};



uint64_t
sum_money_in_outputs(const transaction& tx)
{
    uint64_t sum_xmr {0};

    for (const tx_out& txout: tx.vout)
    {
        sum_xmr += txout.amount;
    }

    return sum_xmr;
}

pair<uint64_t, uint64_t>
sum_money_in_outputs(const string& json_str)
{
    pair<uint64_t, uint64_t> sum_xmr {0, 0};

    json j;

    try
    {
       j = json::parse( json_str);
    }
    catch (std::invalid_argument& e)
    {
        cerr << "sum_money_in_outputs: " << e.what() << endl;
        return sum_xmr;
    }

    for (json& vout: j["vout"])
    {
        sum_xmr.first += vout["amount"].get<uint64_t>();
        ++sum_xmr.second;
    }


    return sum_xmr;
};



uint64_t
sum_money_in_inputs(const transaction& tx)
{
    uint64_t sum_xmr {0};

    size_t input_no = tx.vin.size();

    for (size_t i = 0; i < input_no; ++i)
    {

        if(tx.vin[i].type() != typeid(cryptonote::txin_to_key))
        {
            continue;
        }

        // get tx input key
        const cryptonote::txin_to_key& tx_in_to_key
                = boost::get<cryptonote::txin_to_key>(tx.vin[i]);

        sum_xmr += tx_in_to_key.amount;
    }

    return sum_xmr;
}

pair<uint64_t, uint64_t>
sum_money_in_inputs(const string& json_str)
{
    pair<uint64_t, uint64_t> sum_xmr {0, 0};

    json j;
    try
    {
        j = json::parse( json_str);
    }
    catch (std::invalid_argument& e)
    {
        cerr << "sum_money_in_outputs: " << e.what() << endl;
        return sum_xmr;
    }

    for (json& vin: j["vin"])
    {
        sum_xmr.first += vin["key"]["amount"].get<uint64_t>();
        ++sum_xmr.second;
    }

    return sum_xmr;
};

array<uint64_t, 2>
sum_money_in_tx(const transaction& tx)
{
    array<uint64_t, 2> sum_xmr;

    sum_xmr[0] = sum_money_in_inputs(tx);
    sum_xmr[1] = sum_money_in_outputs(tx);

    return sum_xmr;
};


array<uint64_t, 2>
sum_money_in_txs(const vector<transaction>& txs)
{
    array<uint64_t, 2> sum_xmr {0,0};

    for (const transaction& tx: txs)
    {
        sum_xmr[0] += sum_money_in_inputs(tx);
        sum_xmr[1] += sum_money_in_outputs(tx);
    }

    return sum_xmr;
};


uint64_t
sum_fees_in_txs(const vector<transaction>& txs)
{
    uint64_t fees_sum {0};

    for (const transaction& tx: txs)
    {
        fees_sum += get_tx_fee(tx);
    }

    return fees_sum;
}



vector<pair<txout_to_key, uint64_t>>
get_ouputs(const transaction& tx)
{
    vector<pair<txout_to_key, uint64_t>> outputs;

    for (const tx_out& txout: tx.vout)
    {
        if (txout.target.type() != typeid(txout_to_key))
        {
            continue;
        }

        // get tx input key
        const txout_to_key& txout_key
                = boost::get<cryptonote::txout_to_key>(txout.target);

        outputs.push_back(make_pair(txout_key, txout.amount));
    }

    return outputs;

};

vector<tuple<txout_to_key, uint64_t, uint64_t>>
get_ouputs_tuple(const transaction& tx)
{
    vector<tuple<txout_to_key, uint64_t, uint64_t>> outputs;

    for (uint64_t n = 0; n < tx.vout.size(); ++n)
    {

        if (tx.vout[n].target.type() != typeid(txout_to_key))
        {
            continue;
        }

        // get tx input key
        const txout_to_key& txout_key
                = boost::get<cryptonote::txout_to_key>(tx.vout[n].target);

        outputs.push_back(make_tuple(txout_key, tx.vout[n].amount, n));
    }

    return outputs;
};

uint64_t
get_mixin_no(const transaction& tx)
{
    uint64_t mixin_no {0};

    size_t input_no = tx.vin.size();

    for (size_t i = 0; i < input_no; ++i)
    {

        if(tx.vin[i].type() != typeid(cryptonote::txin_to_key))
        {
            continue;
        }

        // get tx input key
        const txin_to_key& tx_in_to_key
                = boost::get<cryptonote::txin_to_key>(tx.vin[i]);

        mixin_no = tx_in_to_key.key_offsets.size();

        // look for first mixin number.
        // all inputs in a single transaction have same number
        if (mixin_no > 0)
        {
            break;
        }
    }

    return mixin_no;
}
vector<uint64_t>
get_mixin_no(const string& json_str)
{
    vector<uint64_t> mixin_no;

    json j;

    try
    {
        j = json::parse(json_str);

        mixin_no.push_back(j["vin"].at(0)["key"]["key_offsets"].size());
    }
    catch (std::invalid_argument& e)
    {
        cerr << "get_mixin_no: " << e.what() << endl;
        return mixin_no;
    }

    return mixin_no;
}



vector<uint64_t>
get_mixin_no_in_txs(const vector<transaction>& txs)
{
    vector<uint64_t> mixin_no;

    for (const transaction& tx: txs)
    {
        mixin_no.push_back(get_mixin_no(tx));
    }

    return mixin_no;
}


vector<txin_to_key>
get_key_images(const transaction& tx)
{
    vector<txin_to_key> key_images;

    size_t input_no = tx.vin.size();

    for (size_t i = 0; i < input_no; ++i)
    {

        if(tx.vin[i].type() != typeid(txin_to_key))
        {
            continue;
        }

        // get tx input key
        const txin_to_key& tx_in_to_key
                = boost::get<cryptonote::txin_to_key>(tx.vin[i]);

        key_images.push_back(tx_in_to_key);
    }

    return key_images;
}

bool
get_payment_id(const vector<uint8_t>& extra,
               crypto::hash& payment_id,
               crypto::hash8& payment_id8)
{
    payment_id = null_hash;
    payment_id8 = null_hash8;

    std::vector<tx_extra_field> tx_extra_fields;

    if(!parse_tx_extra(extra, tx_extra_fields))
    {
        return false;
    }

    tx_extra_nonce extra_nonce;

    if (find_tx_extra_field_by_type(tx_extra_fields, extra_nonce))
    {
        // first check for encrypted id and then for normal one
        if(get_encrypted_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id8))
        {
            return true;
        }
        else if (get_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id))
        {
            return true;
        }
    }

    return false;
}


bool
get_payment_id(const transaction& tx,
               crypto::hash& payment_id,
               crypto::hash8& payment_id8)
{
    return get_payment_id(tx.extra, payment_id, payment_id8);
}


array<size_t, 5>
timestamp_difference(uint64_t t1, uint64_t t2)
{

    uint64_t timestamp_diff = t1 - t2;

    // calculate difference of timestamps from current block to the mixin one
    if (t2 > t1)
    {
        timestamp_diff = t2 - t1;
    }

    uint64_t time_diff_years = timestamp_diff / 31536000;

    timestamp_diff -=  time_diff_years * 31536000;

    uint64_t time_diff_days = timestamp_diff / 86400;

    timestamp_diff -=  time_diff_days * 86400;

    uint64_t time_diff_hours = timestamp_diff / 3600;

    timestamp_diff -=  time_diff_hours * 3600;

    uint64_t time_diff_minutes = timestamp_diff / 60;

    timestamp_diff -=  time_diff_minutes * 60;

    uint64_t time_diff_seconds = timestamp_diff ;

    return array<size_t, 5> {time_diff_years, time_diff_days,
                             time_diff_hours, time_diff_minutes,
                             time_diff_seconds};

};


string
read(string filename)
{
   if (!bf::exists(bf::path(filename)))
   {
       cerr << "File does not exist: " << filename << endl;
       return string();
   }

    std::ifstream t(filename);
    return string(std::istreambuf_iterator<char>(t),
                  std::istreambuf_iterator<char>());
}

pair<string, double>
timestamps_time_scale(const vector<uint64_t>& timestamps,
                      uint64_t timeN,
                      uint64_t resolution,
                      uint64_t time0)
{
    string empty_time =  string(resolution, '_');

    size_t time_axis_length = empty_time.size();

    uint64_t interval_length = timeN-time0;

    double scale = double(interval_length) / double(time_axis_length);

    for (const auto& timestamp: timestamps)
    {

        if (timestamp < time0 || timestamp > timeN)
        {
            cout << "Out of range" << endl;
            continue;
        }

        uint64_t timestamp_place = double(timestamp-time0)
                         / double(interval_length)*(time_axis_length - 1);

        empty_time[timestamp_place + 1] = '*';
    }

    return make_pair(empty_time, scale);
}

bool
decode_ringct(const rct::rctSig& rv,
              const crypto::public_key &pub,
              const crypto::secret_key &sec,
              unsigned int i,
              rct::key & mask,
              uint64_t & amount)
{
    crypto::key_derivation derivation;

    bool r = crypto::generate_key_derivation(pub, sec, derivation);

    if (!r)
    {
        cerr <<"Failed to generate key derivation to decode rct output " << i << endl;
        return false;
    }

    crypto::secret_key scalar1;

    crypto::derivation_to_scalar(derivation, i, scalar1);

    try
    {
        switch (rv.type)
        {
            case rct::RCTTypeSimple:
            case rct::RCTTypeSimpleBulletproof:
                amount = rct::decodeRctSimple(rv,
                                              rct::sk2rct(scalar1),
                                              i,
                                              mask,
                                              hw::get_device("default"));
                break;
            case rct::RCTTypeFull:
            case rct::RCTTypeFullBulletproof:
                amount = rct::decodeRct(rv,
                                        rct::sk2rct(scalar1),
                                        i,
                                        mask,
                                        hw::get_device("default"));
                break;
            default:
                cerr << "Unsupported rct type: " << rv.type << endl;
                return false;
        }
    }
    catch (const std::exception &e)
    {
        cerr << "Failed to decode input " << i << endl;
        return false;
    }

    return true;
}

bool
url_decode(const std::string& in, std::string& out)
{
    out.clear();
    out.reserve(in.size());
    for (std::size_t i = 0; i < in.size(); ++i)
    {
        if (in[i] == '%')
        {
            if (i + 3 <= in.size())
            {
                int value = 0;
                std::istringstream is(in.substr(i + 1, 2));
                if (is >> std::hex >> value)
                {
                    out += static_cast<char>(value);
                    i += 2;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
        else if (in[i] == '+')
        {
            out += ' ';
        }
        else
        {
            out += in[i];
        }
    }
    return true;
}

map<std::string, std::string>
parse_crow_post_data(const string& req_body)
{
    map<std::string, std::string> body;

    vector<string> vec;
    string tmp;
    bool result = url_decode(req_body, tmp);
    if (result)
    {
        boost::algorithm::split(vec, tmp, [](char x) {return x == '&'; });
        for(auto &it : vec)
        {
            auto pos = it.find("=");
            if (pos != string::npos)
            {
                body[it.substr(0, pos)] = it.substr(pos + 1);
            }
            else
            {
                break;
            }
        }
    }
    return body;
}

// based on
// crypto::public_key wallet2::get_tx_pub_key_from_received_outs(const tools::wallet2::transfer_details &td) const
public_key
get_tx_pub_key_from_received_outs(const transaction &tx)
{
    std::vector<tx_extra_field> tx_extra_fields;

    if(!parse_tx_extra(tx.extra, tx_extra_fields))
    {
        // Extra may only be partially parsed, it's OK if tx_extra_fields contains public key
    }

    // Due to a previous bug, there might be more than one tx pubkey in extra, one being
    // the result of a previously discarded signature.
    // For speed, since scanning for outputs is a slow process, we check whether extra
    // contains more than one pubkey. If not, the first one is returned. If yes, they're
    // checked for whether they yield at least one output
    tx_extra_pub_key pub_key_field;

    if (!find_tx_extra_field_by_type(tx_extra_fields, pub_key_field, 0))
    {
        return null_pkey;
    }

    public_key tx_pub_key = pub_key_field.pub_key;

    bool two_found = find_tx_extra_field_by_type(tx_extra_fields, pub_key_field, 1);

    if (!two_found)
    {
        // easy case, just one found
        return tx_pub_key;
    }
    else
    {
        // just return second one if there are two.
        // this does not require private view key, as
        // its not needed for my use case.
        return pub_key_field.pub_key;
    }

    return null_pkey;
}



string
xmr_amount_to_str(const uint64_t& xmr_amount, string format)
{
    return fmt::format("{:0.12f}", XMR_AMOUNT(xmr_amount));
}



/**
* Check if given output (specified by output_index)
* belongs is ours based
* on our private view key and public spend key
*/
bool
is_output_ours(const size_t& output_index,
               const transaction& tx,
               const public_key& pub_tx_key,
               const secret_key& private_view_key,
               const public_key& public_spend_key)
{
    // public transaction key is combined with our viewkey
    // to create, so called, derived key.
    key_derivation derivation;

    if (!generate_key_derivation(pub_tx_key, private_view_key, derivation))
    {
        cerr << "Cant get dervied key for: "  << "\n"
             << "pub_tx_key: " << pub_tx_key  << " and "
             << "prv_view_key" << private_view_key << endl;

        return false;
    }


    // get the tx output public key
    // that normally would be generated for us,
    // if someone had sent us some xmr.
    public_key pubkey;

    derive_public_key(derivation,
                      output_index,
                      public_spend_key,
                      pubkey);

    //cout << "\n" << tx.vout.size() << " " << output_index << endl;

    // get tx output public key
    const txout_to_key tx_out_to_key
            = boost::get<txout_to_key>(tx.vout[output_index].target);


    if (tx_out_to_key.key == pubkey)
    {
        return true;
    }

    return false;
}


bool
get_real_output_for_key_image(const key_image& ki,
                              const transaction& tx,
                              const secret_key& private_view_key,
                              const public_key& public_spend_key,
                              uint64_t output_idx,
                              public_key output_pub_key)
{



    return false;
}


bool
make_tx_from_json(const string& json_str, transaction& tx)
{
    json j;

    try
    {
        j = json::parse(json_str);
    }
    catch (std::invalid_argument& e)
    {
        cerr << "make_tx_from_json: cant parse json string: " << e.what() << endl;
        return false;
    }

    // get version and unlock time from json
    tx.version     = j["version"].get<size_t>();
    tx.unlock_time = j["unlock_time"].get<uint64_t>();

    // next get extra data
    for (json& extra_val: j["extra"])
        tx.extra.push_back(extra_val.get<uint8_t>());


    // now populate output data from json
    vector<tx_out>& tx_outputs = tx.vout;

    for (json& vo: j["vout"])
    {
        uint64_t amount = vo["amount"].get<uint64_t>();

        public_key out_pub_key;

        if (!epee::string_tools::hex_to_pod(vo["target"]["key"], out_pub_key))
        {
            cerr << "Faild to parse public_key of an output from json" << endl;
            return false;
        }

        txout_target_v target {txout_to_key {out_pub_key}};

        tx_outputs.push_back(tx_out {amount, target});
    }

    // now populate input data from json
    vector<txin_v>& tx_inputs = tx.vin;

    for (json& vi: j["vin"])
    {
        uint64_t amount = vi["key"]["amount"].get<uint64_t>();

        key_image in_key_image;

        if (!epee::string_tools::hex_to_pod(vi["key"]["k_image"], in_key_image))
        {
            cerr << "Faild to parse key_image of an input from json" << endl;
            return false;
        }

        vector<uint64_t> key_offsets;

        for (json& ko: vi["key"]["key_offsets"])
        {
            key_offsets.push_back(ko.get<uint64_t>());
        }

        txin_v _txin_v {txin_to_key {amount, key_offsets, in_key_image}};

        tx_inputs.push_back(_txin_v);
    }

    // add ring signatures field

    if (j.find("signatures") != j.end())
    {
        vector<vector<signature>>& signatures = tx.signatures;

        for (json& sigs: j["signatures"])
        {
            string concatanted_sig = sigs;

            vector<signature> sig_split;

            auto split_sig = [&](string::iterator &b, string::iterator &e)
            {
                signature a_sig;

                if (!epee::string_tools::hex_to_pod(string(b, e), a_sig))
                {
                    cerr << "Faild to parse signature from json" << endl;
                    return false;
                }

                sig_split.push_back(a_sig);

                return true;
            };

            chunks(concatanted_sig.begin(), concatanted_sig.end(), 128, split_sig);

            signatures.push_back(sig_split);
        }

    }

    // now add rct_signatures from json to tx if exist

    if (j.find("rct_signatures") != j.end())
    {
        rct::rctSig& rct_signatures = tx.rct_signatures;

        if (j["rct_signatures"].find("pseudoOuts") != j["rct_signatures"].end())
        {
            rct::keyV& pseudoOuts = rct_signatures.pseudoOuts;

            for (json& pOut: j["rct_signatures"]["pseudoOuts"])
            {
                rct::key pOut_key;

                if (!epee::string_tools::hex_to_pod(pOut, pOut_key))
                {
                    cerr << "Faild to parse pOut_key of pseudoOuts from json" << endl;
                    return false;
                }

                pseudoOuts.push_back(pOut_key);
            }
        }

        vector<rct::ecdhTuple>& ecdhInfo = rct_signatures.ecdhInfo;

        for (json& ecdhI: j["rct_signatures"]["ecdhInfo"])
        {
            rct::ecdhTuple a_tuple;

            //cout << "ecdhI[\"amount\"]: " << ecdhI["amount"] << endl;

            if (!epee::string_tools::hex_to_pod(ecdhI["amount"], a_tuple.amount))
            {
                cerr << "Faild to parse ecdhInfo of an amount from json" << endl;
                return false;
            }

            //cout << epee::string_tools::pod_to_hex(a_tuple.amount) << endl;

            if (!epee::string_tools::hex_to_pod(ecdhI["mask"], a_tuple.mask))
            {
                cerr << "Faild to parse ecdhInfo of an mask from json" << endl;
                return false;
            }

            ecdhInfo.push_back(a_tuple);
        }

        vector<rct::ctkey>& outPk = rct_signatures.outPk;

        for (json& pk: j["rct_signatures"]["outPk"])
        {
            outPk.push_back(rct::ctkey {rct::zero(), rct::zero()});

            rct::key& mask = outPk.back().mask;

            if (!epee::string_tools::hex_to_pod(pk, mask))
            {
                cerr << "Faild to parse rct::key of an outPk from json" << endl;
                return false;
            }

            // cout << "dest: " << epee::string_tools::pod_to_hex(outPk.back().mask) << endl;
        }

        rct_signatures.txnFee = j["rct_signatures"]["txnFee"].get<uint64_t>();
        rct_signatures.type   = j["rct_signatures"]["type"].get<uint8_t>();

    } //  if (j.find("rct_signatures") != j.end())


    if (j.find("rctsig_prunable") != j.end())
    {
        rct::rctSigPrunable& rctsig_prunable = tx.rct_signatures.p;

        vector<rct::rangeSig>& range_sigs = rctsig_prunable.rangeSigs;

        for (json& range_s: j["rctsig_prunable"]["rangeSigs"])
        {
            rct::boroSig asig;

            if (!epee::string_tools::hex_to_pod(range_s["asig"], asig))
            {
                cerr << "Faild to parse asig of an asnlSig from json" << endl;
                return false;
            }


            struct {
                rct::key64 Ci;
            } key64_contained;

            if (!epee::string_tools::hex_to_pod(range_s["Ci"], key64_contained))
            {
                cerr << "Faild to parse Ci of an asnlSig from json" << endl;
                return false;
            }

            range_sigs.push_back(rct::rangeSig {});

            rct::rangeSig& last_range_sig = range_sigs.back();

            last_range_sig.asig = asig;

            memcpy(&(last_range_sig.Ci), &(key64_contained.Ci), sizeof(rct::key64));
        }

        vector<rct::mgSig>& mg_sigs = rctsig_prunable.MGs;

        for (json& a_mgs: j["rctsig_prunable"]["MGs"])
        {
            rct::mgSig new_mg_sig;

            vector<rct::keyV>& ss = new_mg_sig.ss;

            for (json& ss_j: a_mgs["ss"])
            {
                rct::key a_key1;

                if (!epee::string_tools::hex_to_pod(ss_j[0], a_key1))
                {
                    cerr << "Faild to parse ss a_key1 of an MGs from json" << endl;
                    return false;
                }

                rct::key a_key2;

                if (!epee::string_tools::hex_to_pod(ss_j[1], a_key2))
                {
                    cerr << "Faild to parse ss a_key2 of an MGs from json" << endl;
                    return false;
                }

                ss.push_back(vector<rct::key>{a_key1, a_key2});
            }

            json& cc_j = a_mgs["cc"];

            if (!epee::string_tools::hex_to_pod(cc_j, new_mg_sig.cc))
            {
                cerr << "Faild to parse cc an MGs from json" << endl;
                return false;
            }

            mg_sigs.push_back(new_mg_sig);
        }

    } // j.find("rctsig_prunable") != j.end()


    //cout << j.dump(4) << endl;

    //cout << "From reconstructed tx: " << obj_to_json_str(tx) << endl;

    return true;
}

string
get_human_readable_timestamp(uint64_t ts)
{
    char buffer[64];
    if (ts < 1234567890)
        return "<unknown>";

    time_t tt = ts;

    struct tm tm;

    gmtime_r(&tt, &tm);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %I:%M:%S ", &tm);

    return std::string(buffer);
}

string
make_hash(const string& in_str)
{
    crypto::hash vk_hash;
    crypto::cn_fast_hash(in_str.c_str(), in_str.length(), vk_hash);
    return pod_to_hex(vk_hash);
}


bool
hex_to_tx(string const& tx_hex, transaction& tx, crypto::hash& tx_hash,  crypto::hash& tx_prefix_hash)
{
    std::string tx_blob;

    epee::string_tools::parse_hexstr_to_binbuff(tx_hex, tx_blob);

    return parse_and_validate_tx_from_blob(tx_blob, tx, tx_hash, tx_prefix_hash);
}

string
tx_to_hex(transaction const& tx)
{
    return epee::string_tools::buff_to_hex_nodelimer(t_serializable_object_to_blob(tx));
}

string
hex_to_tx_blob(string const& tx_hex)
{
    std::string tx_blob;

    epee::string_tools::parse_hexstr_to_binbuff(tx_hex, tx_blob);

    return tx_blob;
}

bool
hex_to_complete_block(string const& cblk_str,
                      block_complete_entry& cblk)
{
    cryptonote::blobdata cblk_blob;

    if (!epee::string_tools::parse_hexstr_to_binbuff(
                cblk_str, cblk_blob))
        return false;

    if (!epee::serialization::load_t_from_binary(cblk, cblk_blob))
        return false;

    return true;
}

bool
hex_to_complete_block(vector<string> const& cblks_str,
                      vector<block_complete_entry>& cblks)
{
    for (auto const& cblk_str: cblks_str)
    {

        block_complete_entry cblk;

        if (!hex_to_complete_block(cblk_str, cblk))
            return false;

        cblks.push_back(cblk);
    }

    return true;
}

bool
blocks_and_txs_from_complete_blocks(
        vector<block_complete_entry> const& cblks,
        vector<block>& blocks,
        vector<transaction>& transactions)
{


    for (auto const& cblk: cblks)
    {
        block blk;

        if (!parse_and_validate_block_from_blob(cblk.block, blk))
            return false;

        blocks.push_back(blk);

        // first is miner_tx
        transactions.push_back(blk.miner_tx);

        vector<transaction> txs;

        for (auto const& tx_blob: cblk.txs)
        {
            transaction tx;

            if (!parse_and_validate_tx_from_blob(tx_blob, tx))
                return false;

            txs.push_back(tx);
        }

        // now normal txs
        transactions.insert(transactions.end(),
                            txs.begin(), txs.end());
    }

    return true;
}


}

