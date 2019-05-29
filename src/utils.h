//
// Created by mwo on 5/11/15.
//

#ifndef XMREG01_TOOLS_H
#define XMREG01_TOOLS_H

#define PATH_SEPARARTOR '/'

#define XMR_AMOUNT(value) \
    static_cast<double>(value) / 1e12

#define REMOVE_HASH_BRAKETS(a_hash) \
    a_hash.substr(1, a_hash.size()-2)

#include "src/monero_headers.h"

#include "ext/json.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>


#include <string>
#include <vector>
#include <array>
#include <random>

/**
 * Some helper functions that might or might not be useful in this project.
 * Names are rather self-explanatory, so I think
 * there is no reason for any detailed explanations here
 */
namespace xmreg
{

using namespace cryptonote;
using namespace crypto;
using namespace std;

namespace bf = boost::filesystem;
//namespace pt = boost::posix_time;
//namespace gt = boost::gregorian;


using json = nlohmann::json;

using  epee::string_tools::pod_to_hex;
using  epee::string_tools::hex_to_pod;


bool
get_tx_pub_key_from_str_hash(Blockchain& core_storage,
                         const string& hash_str,
                         transaction& tx);

inline bool
is_separator(char c);

string
print_sig (const signature& sig);


string
timestamp_to_str_gm(time_t timestamp, const char* format = "%F %T");

ostream&
operator<< (ostream& os, const address_parse_info& addr);


bool
generate_key_image(const crypto::key_derivation& derivation,
                   const std::size_t output_index,
                   const crypto::secret_key& sec_key,
                   const crypto::public_key& pub_key,
                   crypto::key_image& key_img);



array<uint64_t, 4>
summary_of_in_out_rct(
        const transaction& tx,
        vector<pair<txout_to_key, uint64_t>>& output_pub_keys,
        vector<txin_to_key>& input_key_imgs);

// this version for mempool txs from json
array<uint64_t, 6>
summary_of_in_out_rct(const json& _json);

uint64_t
sum_money_in_outputs(const transaction& tx);

pair<uint64_t, uint64_t>
sum_money_in_outputs(const string& json_str);

uint64_t
sum_money_in_inputs(const transaction& tx);

pair<uint64_t, uint64_t>
sum_money_in_inputs(const string& json_str);

array<uint64_t, 2>
sum_money_in_tx(const transaction& tx);

array<uint64_t, 2>
sum_money_in_txs(const vector<transaction>& txs);

uint64_t
sum_fees_in_txs(const vector<transaction>& txs);

uint64_t
get_mixin_no(const transaction& tx);

vector<uint64_t>
get_mixin_no(const string& json_str);

vector<uint64_t>
get_mixin_no_in_txs(const vector<transaction>& txs);

vector<pair<txout_to_key, uint64_t>>
get_ouputs(const transaction& tx);

vector<tuple<txout_to_key, uint64_t, uint64_t>>
get_ouputs_tuple(const transaction& tx);

vector<txin_to_key>
get_key_images(const transaction& tx);

bool
get_payment_id(const vector<uint8_t>& extra,
               crypto::hash& payment_id,
               crypto::hash8& payment_id8);


inline bool
get_payment_id(const transaction& tx,
               crypto::hash& payment_id,
               crypto::hash8& payment_id8)
{
    return get_payment_id(tx.extra, payment_id, payment_id8);
}

inline tuple<crypto::hash, crypto::hash8>
get_payment_id(transaction const& tx)
{
    crypto::hash payment_id;
    crypto::hash8 payment_id8;

    get_payment_id(tx.extra, payment_id, payment_id8);

    return make_tuple(payment_id, payment_id8);
}

// Encryption and decryption are the same operation (xor with a key)
bool
encrypt_payment_id(crypto::hash8 &payment_id,
                   const crypto::public_key &public_key,
                   const crypto::secret_key &secret_key);


inline double
get_xmr(uint64_t core_amount)
{
    return  static_cast<double>(core_amount) / 1e12;
}

array<size_t, 5>
timestamp_difference(uint64_t t1, uint64_t t2);

string
read(string filename);



pair<string, double>
timestamps_time_scale(const vector<uint64_t>& timestamps,
                  uint64_t timeN, uint64_t resolution = 80,
                  uint64_t time0 = 1397818193 /* timestamp of the second block */);



bool
decode_ringct(const rct::rctSig & rv,
              const crypto::public_key &pub,
              const crypto::secret_key &sec,
              unsigned int i,
              rct::key & mask,
              uint64_t & amount);

bool
decode_ringct(rct::rctSig const& rv,
              crypto::key_derivation const& derivation,
              unsigned int i,
              rct::key& mask,
              uint64_t& amount);
bool
url_decode(const std::string& in, std::string& out);

map<std::string, std::string>
parse_crow_post_data(const string& req_body);



//string
//xmr_amount_to_str(const uint64_t& xmr_amount, string format="{:0.12f}");

bool
is_output_ours(const size_t& output_index,
               const transaction& tx,
               const public_key& pub_tx_key,
               const secret_key& private_view_key,
               const public_key& public_spend_key);


// based on http://stackoverflow.com/a/9943098/248823
template<typename Iterator, typename Func>
void chunks(Iterator begin,
            Iterator end,
            iterator_traits<string::iterator>::difference_type k,
            Func f)
{
    Iterator chunk_begin;
    Iterator chunk_end;
    chunk_end = chunk_begin = begin;

    do
    {
        if(std::distance(chunk_end, end) < k)
            chunk_end = end;
        else
            std::advance(chunk_end, k);
        f(chunk_begin,chunk_end);
        chunk_begin = chunk_end;
    }
    while(std::distance(chunk_begin,end) > 0);
}

bool
make_tx_from_json(const string& json_str, transaction& tx);


inline
crypto::hash
generated_payment_id()
{
    return crypto::rand<crypto::hash>();
}

string
get_human_readable_timestamp(uint64_t ts);


string
make_hash(const string& in_str);


bool
hex_to_tx_blob(string const& tx_hex, string& tx_blob);

bool
hex_to_complete_block(string const& cblk_str,
                      block_complete_entry& cblk);

bool
hex_to_complete_block(vector<string> const& cblks_str,
                      vector<block_complete_entry> & cblks);

bool
blocks_and_txs_from_complete_blocks(
        vector<block_complete_entry> const& cblks,
        vector<block>& blocks,
        vector<transaction>& txs);

// this function only useful in google test for mocking
// ring member output info
bool
output_data_from_hex(
        string const& out_data_hex,
        std::map<vector<uint64_t>,
                 vector<cryptonote::output_data_t>>& outputs_data_map);

// this function only useful in google test for mocking
// known outputs and their amounts
bool
populate_known_outputs_from_csv(
        string const& csv_file,
        std::unordered_map<public_key, uint64_t>& known_outputs,
        bool skip_first_line = true);
}

#endif //XMREG01_TOOLS_H
