// Copyright (c) 2014-2017, MyMonero.com
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Original Author: Lucas Jones
// Modified by luigi1111 2017

var cnUtil = (function(initConfig) {
    var config = $.extend({}, initConfig);
    config.coinUnits = new JSBigInt(10).pow(config.coinUnitPlaces);

    var HASH_STATE_BYTES = 200;
    var HASH_SIZE = 32;
    var ADDRESS_CHECKSUM_SIZE = 4;
    var INTEGRATED_ID_SIZE = 8;
    var ENCRYPTED_PAYMENT_ID_TAIL = 141;
    var CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX = config.addressPrefix;
    var CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX = config.integratedAddressPrefix;
    var CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX = config.subAddressPrefix;


    if (config.nettype === 1 /*testnet*/) {
        CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX = config.addressPrefixTestnet;
        CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX = config.integratedAddressPrefixTestnet;
        CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX = config.subAddressPrefixTestnet;
    } else if (config.nettype === 2 /*stagenet*/) {
        CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX = config.addressPrefixStagenet;
        CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX = config.integratedAddressPrefixStagenet;
        CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX = config.subAddressPrefixStagenet;
}

    var UINT64_MAX = new JSBigInt(2).pow(64);
    var CURRENT_TX_VERSION = 2;
    var OLD_TX_VERSION = 1;
    var RCTTypeFull = 1;
    var RCTTypeSimple = 2;
    var TX_EXTRA_NONCE_MAX_COUNT = 255;
    var TX_EXTRA_TAGS = {
        PADDING: '00',
        PUBKEY: '01',
        NONCE: '02',
        MERGE_MINING: '03'
    };
    var TX_EXTRA_NONCE_TAGS = {
        PAYMENT_ID: '00',
        ENCRYPTED_PAYMENT_ID: '01'
    };
    var KEY_SIZE = 32;
    var STRUCT_SIZES = {
        GE_P3: 160,
        GE_P2: 120,
        GE_P1P1: 160,
        GE_CACHED: 160,
        EC_SCALAR: 32,
        EC_POINT: 32,
        KEY_IMAGE: 32,
        GE_DSMP: 160 * 8, // ge_cached * 8
        SIGNATURE: 64 // ec_scalar * 2
    };

    //RCT vars
    var H = "8b655970153799af2aeadc9ff1add0ea6c7251d54154cfa92c173a0dd39c1f94"; //base H for amounts
    var l = JSBigInt("7237005577332262213973186563042994240857116359379907606001950938285454250989"); //curve order (not RCT specific)
    var I = "0100000000000000000000000000000000000000000000000000000000000000"; //identity element
    var Z = "0000000000000000000000000000000000000000000000000000000000000000"; //zero scalar
    //H2 object to speed up some operations
    var H2 = ["8b655970153799af2aeadc9ff1add0ea6c7251d54154cfa92c173a0dd39c1f94", "8faa448ae4b3e2bb3d4d130909f55fcd79711c1c83cdbccadd42cbe1515e8712",
        "12a7d62c7791654a57f3e67694ed50b49a7d9e3fc1e4c7a0bde29d187e9cc71d", "789ab9934b49c4f9e6785c6d57a498b3ead443f04f13df110c5427b4f214c739",
        "771e9299d94f02ac72e38e44de568ac1dcb2edc6edb61f83ca418e1077ce3de8", "73b96db43039819bdaf5680e5c32d741488884d18d93866d4074a849182a8a64",
        "8d458e1c2f68ebebccd2fd5d379f5e58f8134df3e0e88cad3d46701063a8d412", "09551edbe494418e81284455d64b35ee8ac093068a5f161fa6637559177ef404",
        "d05a8866f4df8cee1e268b1d23a4c58c92e760309786cdac0feda1d247a9c9a7", "55cdaad518bd871dd1eb7bc7023e1dc0fdf3339864f88fdd2de269fe9ee1832d",
        "e7697e951a98cfd5712b84bbe5f34ed733e9473fcb68eda66e3788df1958c306", "f92a970bae72782989bfc83adfaa92a4f49c7e95918b3bba3cdc7fe88acc8d47",
        "1f66c2d491d75af915c8db6a6d1cb0cd4f7ddcd5e63d3ba9b83c866c39ef3a2b", "3eec9884b43f58e93ef8deea260004efea2a46344fc5965b1a7dd5d18997efa7",
        "b29f8f0ccb96977fe777d489d6be9e7ebc19c409b5103568f277611d7ea84894", "56b1f51265b9559876d58d249d0c146d69a103636699874d3f90473550fe3f2c",
        "1d7a36575e22f5d139ff9cc510fa138505576b63815a94e4b012bfd457caaada", "d0ac507a864ecd0593fa67be7d23134392d00e4007e2534878d9b242e10d7620",
        "f6c6840b9cf145bb2dccf86e940be0fc098e32e31099d56f7fe087bd5deb5094", "28831a3340070eb1db87c12e05980d5f33e9ef90f83a4817c9f4a0a33227e197",
        "87632273d629ccb7e1ed1a768fa2ebd51760f32e1c0b867a5d368d5271055c6e", "5c7b29424347964d04275517c5ae14b6b5ea2798b573fc94e6e44a5321600cfb",
        "e6945042d78bc2c3bd6ec58c511a9fe859c0ad63fde494f5039e0e8232612bd5", "36d56907e2ec745db6e54f0b2e1b2300abcb422e712da588a40d3f1ebbbe02f6",
        "34db6ee4d0608e5f783650495a3b2f5273c5134e5284e4fdf96627bb16e31e6b", "8e7659fb45a3787d674ae86731faa2538ec0fdf442ab26e9c791fada089467e9",
        "3006cf198b24f31bb4c7e6346000abc701e827cfbb5df52dcfa42e9ca9ff0802", "f5fd403cb6e8be21472e377ffd805a8c6083ea4803b8485389cc3ebc215f002a",
        "3731b260eb3f9482e45f1c3f3b9dcf834b75e6eef8c40f461ea27e8b6ed9473d", "9f9dab09c3f5e42855c2de971b659328a2dbc454845f396ffc053f0bb192f8c3",
        "5e055d25f85fdb98f273e4afe08464c003b70f1ef0677bb5e25706400be620a5", "868bcf3679cb6b500b94418c0b8925f9865530303ae4e4b262591865666a4590",
        "b3db6bd3897afbd1df3f9644ab21c8050e1f0038a52f7ca95ac0c3de7558cb7a", "8119b3a059ff2cac483e69bcd41d6d27149447914288bbeaee3413e6dcc6d1eb",
        "10fc58f35fc7fe7ae875524bb5850003005b7f978c0c65e2a965464b6d00819c", "5acd94eb3c578379c1ea58a343ec4fcff962776fe35521e475a0e06d887b2db9",
        "33daf3a214d6e0d42d2300a7b44b39290db8989b427974cd865db011055a2901", "cfc6572f29afd164a494e64e6f1aeb820c3e7da355144e5124a391d06e9f95ea",
        "d5312a4b0ef615a331f6352c2ed21dac9e7c36398b939aec901c257f6cbc9e8e", "551d67fefc7b5b9f9fdbf6af57c96c8a74d7e45a002078a7b5ba45c6fde93e33",
        "d50ac7bd5ca593c656928f38428017fc7ba502854c43d8414950e96ecb405dc3", "0773e18ea1be44fe1a97e239573cfae3e4e95ef9aa9faabeac1274d3ad261604",
        "e9af0e7ca89330d2b8615d1b4137ca617e21297f2f0ded8e31b7d2ead8714660", "7b124583097f1029a0c74191fe7378c9105acc706695ed1493bb76034226a57b",
        "ec40057b995476650b3db98e9db75738a8cd2f94d863b906150c56aac19caa6b", "01d9ff729efd39d83784c0fe59c4ae81a67034cb53c943fb818b9d8ae7fc33e5",
        "00dfb3c696328c76424519a7befe8e0f6c76f947b52767916d24823f735baf2e", "461b799b4d9ceea8d580dcb76d11150d535e1639d16003c3fb7e9d1fd13083a8",
        "ee03039479e5228fdc551cbde7079d3412ea186a517ccc63e46e9fcce4fe3a6c", "a8cfb543524e7f02b9f045acd543c21c373b4c9b98ac20cec417a6ddb5744e94",
        "932b794bf89c6edaf5d0650c7c4bad9242b25626e37ead5aa75ec8c64e09dd4f", "16b10c779ce5cfef59c7710d2e68441ea6facb68e9b5f7d533ae0bb78e28bf57",
        "0f77c76743e7396f9910139f4937d837ae54e21038ac5c0b3fd6ef171a28a7e4", "d7e574b7b952f293e80dde905eb509373f3f6cd109a02208b3c1e924080a20ca",
        "45666f8c381e3da675563ff8ba23f83bfac30c34abdde6e5c0975ef9fd700cb9", "b24612e454607eb1aba447f816d1a4551ef95fa7247fb7c1f503020a7177f0dd",
        "7e208861856da42c8bb46a7567f8121362d9fb2496f131a4aa9017cf366cdfce", "5b646bff6ad1100165037a055601ea02358c0f41050f9dfe3c95dccbd3087be0",
        "746d1dccfed2f0ff1e13c51e2d50d5324375fbd5bf7ca82a8931828d801d43ab", "cb98110d4a6bb97d22feadbc6c0d8930c5f8fc508b2fc5b35328d26b88db19ae",
        "60b626a033b55f27d7676c4095eababc7a2c7ede2624b472e97f64f96b8cfc0e", "e5b52bc927468df71893eb8197ef820cf76cb0aaf6e8e4fe93ad62d803983104",
        "056541ae5da9961be2b0a5e895e5c5ba153cbb62dd561a427bad0ffd41923199", "f8fef05a3fa5c9f3eba41638b247b711a99f960fe73aa2f90136aeb20329b888"];


    //switch byte order for hex string
    function swapEndian(hex){
        if (hex.length % 2 !== 0){return "length must be a multiple of 2!";}
        var data = "";
        for (var i=1; i <= hex.length / 2; i++){
            data += hex.substr(0 - 2 * i, 2);
        }
        return data;
    }

    //switch byte order charwise
    function swapEndianC(string){
        var data = "";
        for (var i=1; i <= string.length; i++){
            data += string.substr(0 - i, 1);
        }
        return data;
    }

    //for most uses you'll also want to swapEndian after conversion
    //mainly to convert integer "scalars" to usable hexadecimal strings
    function d2h(integer){
        if (typeof integer !== "string" && integer.toString().length > 15){throw "integer should be entered as a string for precision";}
        var padding = "";
        for (i = 0; i < 63; i++){
            padding += "0";
        }
        return (padding + JSBigInt(integer).toString(16).toLowerCase()).slice(-64);
    }

    //integer (string) to scalar
    function d2s(integer){
        return swapEndian(d2h(integer));
    }

    //scalar to integer (string)
    function s2d(scalar){
        return JSBigInt.parse(swapEndian(scalar), 16).toString();
    }

    //convert integer string to 64bit "binary" little-endian string
    function d2b(integer){
        if (typeof integer !== "string" && integer.toString().length > 15){throw "integer should be entered as a string for precision";}
        var padding = "";
        for (i = 0; i < 63; i++){
            padding += "0";
        }
        var a = new JSBigInt(integer);
        if (a.toString(2).length > 64){throw "amount overflows uint64!";}
        return swapEndianC((padding + a.toString(2)).slice(-64));
    }

    //convert integer string to 64bit base 4 little-endian string
    function d2b4(integer){
        if (typeof integer !== "string" && integer.toString().length > 15){throw "integer should be entered as a string for precision";}
        var padding = "";
        for (i = 0; i < 31; i++){
            padding += "0";
        }
        var a = new JSBigInt(integer);
        if (a.toString(2).length > 64){throw "amount overflows uint64!";}
        return swapEndianC((padding + a.toString(4)).slice(-32));
    }
    //end rct new functions

    this.valid_hex = function(hex) {
        var exp = new RegExp("[0-9a-fA-F]{" + hex.length + "}");
        return exp.test(hex);
    };

    //simple exclusive or function for two hex inputs
    this.hex_xor = function(hex1, hex2) {
        if (!hex1 || !hex2 || hex1.length !== hex2.length || hex1.length % 2 !== 0 || hex2.length % 2 !== 0){throw "Hex string(s) is/are invalid!";}
        var bin1 = hextobin(hex1);
        var bin2 = hextobin(hex2);
        var xor = new Uint8Array(bin1.length);
        for (i = 0; i < xor.length; i++){
            xor[i] = bin1[i] ^ bin2[i];
        }
        return bintohex(xor);
    };

    function hextobin(hex) {
        if (hex.length % 2 !== 0) throw "Hex string has invalid length!";
        var res = new Uint8Array(hex.length / 2);
        for (var i = 0; i < hex.length / 2; ++i) {
            res[i] = parseInt(hex.slice(i * 2, i * 2 + 2), 16);
        }
        return res;
    }

    function bintohex(bin) {
        var out = [];
        for (var i = 0; i < bin.length; ++i) {
            out.push(("0" + bin[i].toString(16)).slice(-2));
        }
        return out.join("");
    }

    // Generate a 256-bit crypto random
    this.rand_32 = function() {
        return mn_random(256);
    };

    // Generate a 128-bit crypto random
    this.rand_16 = function() {
        return mn_random(128);
    };

    // Generate a 64-bit crypto random
    this.rand_8 = function() {
        return mn_random(64);
    };

    this.encode_varint = function(i) {
        i = new JSBigInt(i);
        var out = '';
        // While i >= b10000000
        while (i.compare(0x80) >= 0) {
            // out.append i & b01111111 | b10000000
            out += ("0" + ((i.lowVal() & 0x7f) | 0x80).toString(16)).slice(-2);
            i = i.divide(new JSBigInt(2).pow(7));
        }
        out += ("0" + i.toJSValue().toString(16)).slice(-2);
        return out;
    };

    this.sc_reduce = function(hex) {
        var input = hextobin(hex);
        if (input.length !== 64) {
            throw "Invalid input length";
        }
        var mem = Module._malloc(64);
        Module.HEAPU8.set(input, mem);
        Module.ccall('sc_reduce', 'void', ['number'], [mem]);
        var output = Module.HEAPU8.subarray(mem, mem + 64);
        Module._free(mem);
        return bintohex(output);
    };

    this.sc_reduce32 = function(hex) {
        var input = hextobin(hex);
        if (input.length !== 32) {
            throw "Invalid input length";
        }
        var mem = Module._malloc(32);
        Module.HEAPU8.set(input, mem);
        Module.ccall('sc_reduce32', 'void', ['number'], [mem]);
        var output = Module.HEAPU8.subarray(mem, mem + 32);
        Module._free(mem);
        return bintohex(output);
    };

    this.cn_fast_hash = function(input, inlen) {
        /*if (inlen === undefined || !inlen) {
            inlen = Math.floor(input.length / 2);
        }*/
        if (input.length % 2 !== 0 || !this.valid_hex(input)) {
            throw "Input invalid";
        }
        //update to use new keccak impl (approx 45x faster)
        //var state = this.keccak(input, inlen, HASH_STATE_BYTES);
        //return state.substr(0, HASH_SIZE * 2);
        return keccak_256(hextobin(input));
    };

    //many functions below are commented out now, and duplicated with the faster nacl impl --luigi1111
    // to be removed completely later
    /*this.sec_key_to_pub = function(sec) {
        var input = hextobin(sec);
        if (input.length !== 32) {
            throw "Invalid input length";
        }
        var input_mem = Module._malloc(KEY_SIZE);
        Module.HEAPU8.set(input, input_mem);
        var ge_p3 = Module._malloc(STRUCT_SIZES.GE_P3);
        var out_mem = Module._malloc(KEY_SIZE);
        Module.ccall('ge_scalarmult_base', 'void', ['number', 'number'], [ge_p3, input_mem]);
        Module.ccall('ge_p3_tobytes', 'void', ['number', 'number'], [out_mem, ge_p3]);
        var output = Module.HEAPU8.subarray(out_mem, out_mem + KEY_SIZE);
        Module._free(ge_p3);
        Module._free(input_mem);
        Module._free(out_mem);
        return bintohex(output);
    };*/

    this.sec_key_to_pub = function(sec) {
        if (sec.length !== 64) {
            throw "Invalid sec length";
        }
        return bintohex(nacl.ll.ge_scalarmult_base(hextobin(sec)));
    };

    //alias
    this.ge_scalarmult_base = function(sec) {
        return this.sec_key_to_pub(sec);
    };

    //accepts arbitrary point, rather than G
    /*this.ge_scalarmult = function(pub, sec) {
        if (pub.length !== 64 || sec.length !== 64) {
            throw "Invalid input length";
        }
        var pub_b = hextobin(pub);
        var sec_b = hextobin(sec);
        var pub_m = Module._malloc(KEY_SIZE);
        Module.HEAPU8.set(pub_b, pub_m);
        var sec_m = Module._malloc(KEY_SIZE);
        Module.HEAPU8.set(sec_b, sec_m);
        var ge_p3_m = Module._malloc(STRUCT_SIZES.GE_P3);
        var ge_p2_m = Module._malloc(STRUCT_SIZES.GE_P2);
        if (Module.ccall("ge_frombytes_vartime", "bool", ["number", "number"], [ge_p3_m, pub_m]) !== 0) {
            throw "ge_frombytes_vartime returned non-zero error code";
        }
        Module.ccall("ge_scalarmult", "void", ["number", "number", "number"], [ge_p2_m, sec_m, ge_p3_m]);
        var derivation_m = Module._malloc(KEY_SIZE);
        Module.ccall("ge_tobytes", "void", ["number", "number"], [derivation_m, ge_p2_m]);
        var res = Module.HEAPU8.subarray(derivation_m, derivation_m + KEY_SIZE);
        Module._free(pub_m);
        Module._free(sec_m);
        Module._free(ge_p3_m);
        Module._free(ge_p2_m);
        Module._free(derivation_m);
        return bintohex(res);
    };*/
    this.ge_scalarmult = function(pub, sec) {
        if (pub.length !== 64 || sec.length !== 64) {
            throw "Invalid input length";
        }
        return bintohex(nacl.ll.ge_scalarmult(hextobin(pub), hextobin(sec)));
    };

    this.pubkeys_to_string = function(spend, view) {
        var prefix = this.encode_varint(CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX);
        var data = prefix + spend + view;
        var checksum = this.cn_fast_hash(data);
        return cnBase58.encode(data + checksum.slice(0, ADDRESS_CHECKSUM_SIZE * 2));
    };

    this.get_account_integrated_address = function(address, payment_id8) {
        var decoded_address = decode_address(address);

        var prefix = this.encode_varint(CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX);
        var data = prefix + decoded_address.spend  + decoded_address.view + payment_id8;

        var checksum = this.cn_fast_hash(data);

        return cnBase58.encode(data + checksum.slice(0, ADDRESS_CHECKSUM_SIZE * 2));
    };


    this.decrypt_payment_id = function(payment_id8, tx_public_key, acc_prv_view_key) {
        if (payment_id8.length !== 16) throw "Invalid input length!";

        var key_derivation = this.generate_key_derivation(tx_public_key, acc_prv_view_key);

        var pid_key = this.cn_fast_hash(key_derivation + ENCRYPTED_PAYMENT_ID_TAIL.toString(16)).slice(0, INTEGRATED_ID_SIZE * 2);

        var decrypted_payment_id = this.hex_xor(payment_id8, pid_key);

        return decrypted_payment_id;
}

    // Generate keypair from seed
    this.generate_keys = function(seed) {
        if (seed.length !== 64) throw "Invalid input length!";
        var sec = this.sc_reduce32(seed);
        var pub = this.sec_key_to_pub(sec);
        return {
            'sec': sec,
            'pub': pub
        };
    };

    this.random_keypair = function() {
        return this.generate_keys(this.rand_32());
    };

    // Random 32-byte ec scalar
    this.random_scalar = function() {
        //var rand = this.sc_reduce(mn_random(64 * 8));
        //return rand.slice(0, STRUCT_SIZES.EC_SCALAR * 2);
        return this.sc_reduce32(this.rand_32());
    };

    /* no longer used
    this.keccak = function(hex, inlen, outlen) {
        var input = hextobin(hex);
        if (input.length !== inlen) {
            throw "Invalid input length";
        }
        if (outlen <= 0) {
            throw "Invalid output length";
        }
        var input_mem = Module._malloc(inlen);
        Module.HEAPU8.set(input, input_mem);
        var out_mem = Module._malloc(outlen);
        Module._keccak(input_mem, inlen | 0, out_mem, outlen | 0);
        var output = Module.HEAPU8.subarray(out_mem, out_mem + outlen);
        Module._free(input_mem);
        Module._free(out_mem);
        return bintohex(output);
    };*/

    this.is_subaddress = function(address) {
        var dec = cnBase58.decode(address);
        var expectedPrefixSub = this.encode_varint(CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX);
        var prefix = dec.slice(0, expectedPrefixSub.length);
        return (prefix === expectedPrefixSub);
    }

    this.create_address = function(seed) {
        var keys = {};
        var first;
        if (seed.length !== 64) {
            first = this.cn_fast_hash(seed);
        } else {
            first = seed; //only input reduced seeds or this will not give you the result you want
        }
        keys.spend = this.generate_keys(first);

        var second;
        if (seed.length !== 64) {
            second = this.cn_fast_hash(first);
        } else {
            second = this.cn_fast_hash(keys.spend.sec);
        }

        keys.view = this.generate_keys(second);
        keys.public_addr = this.pubkeys_to_string(keys.spend.pub, keys.view.pub);
        return keys;
    };

    this.create_addr_prefix = function(seed) {
        var first;
        if (seed.length !== 64) {
            first = this.cn_fast_hash(seed);
        } else {
            first = seed;
        }
        var spend = this.generate_keys(first);
        var prefix = this.encode_varint(CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX);
        return cnBase58.encode(prefix + spend.pub).slice(0, 44);
    };
    
    this.decode_address = function(address) {
        var dec = cnBase58.decode(address);

        var expectedPrefix = this.encode_varint(CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX);
        var expectedPrefixInt = this.encode_varint(CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX);
        var expectedPrefixSub = this.encode_varint(CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX);


        var prefix = dec.slice(0, expectedPrefix.length);
        if (prefix !== expectedPrefix && prefix !== expectedPrefixInt && prefix !== expectedPrefixSub) {
            throw "Invalid address prefix";
        }
        dec = dec.slice(expectedPrefix.length);
        var spend = dec.slice(0, 64);
        var view = dec.slice(64, 128);
        if (prefix === expectedPrefixInt){
            var intPaymentId = dec.slice(128, 128 + (INTEGRATED_ID_SIZE * 2));
            var checksum = dec.slice(128 + (INTEGRATED_ID_SIZE * 2), 128 + (INTEGRATED_ID_SIZE * 2) + (ADDRESS_CHECKSUM_SIZE * 2));
            var expectedChecksum = this.cn_fast_hash(prefix + spend + view + intPaymentId).slice(0, ADDRESS_CHECKSUM_SIZE * 2);
        } else if (prefix === expectedPrefix) {
            var checksum = dec.slice(128, 128 + (ADDRESS_CHECKSUM_SIZE * 2));
            var expectedChecksum = this.cn_fast_hash(prefix + spend + view).slice(0, ADDRESS_CHECKSUM_SIZE * 2);
        } else {
            // if its not regular address, nor integrated, than it must be subaddress
            var checksum = dec.slice(128, 128 + (ADDRESS_CHECKSUM_SIZE * 2));
            var expectedChecksum = this.cn_fast_hash(prefix + spend + view).slice(0, ADDRESS_CHECKSUM_SIZE * 2);
        }
        if (checksum !== expectedChecksum) {
            throw "Invalid checksum";
        }
        if (intPaymentId){
            return {
                spend: spend,
                view: view,
                intPaymentId: intPaymentId
            };
        } else {
            return {
                spend: spend,
                view: view
            };
        }
};
    this.valid_keys = function(view_pub, view_sec, spend_pub, spend_sec) {
        var expected_view_pub = this.sec_key_to_pub(view_sec);
        var expected_spend_pub = this.sec_key_to_pub(spend_sec);
        return (expected_spend_pub === spend_pub) && (expected_view_pub === view_pub);
    };

    this.hash_to_scalar = function(buf) {
        var hash = this.cn_fast_hash(buf);
        var scalar = this.sc_reduce32(hash);
        return scalar;
    };

    /*this.generate_key_derivation = function(pub, sec) {
        if (pub.length !== 64 || sec.length !== 64) {
            throw "Invalid input length";
        }
        var pub_b = hextobin(pub);
        var sec_b = hextobin(sec);
        var pub_m = Module._malloc(KEY_SIZE);
        Module.HEAPU8.set(pub_b, pub_m);
        var sec_m = Module._malloc(KEY_SIZE);
        Module.HEAPU8.set(sec_b, sec_m);
        var ge_p3_m = Module._malloc(STRUCT_SIZES.GE_P3);
        var ge_p2_m = Module._malloc(STRUCT_SIZES.GE_P2);
        var ge_p1p1_m = Module._malloc(STRUCT_SIZES.GE_P1P1);
        if (Module.ccall("ge_frombytes_vartime", "bool", ["number", "number"], [ge_p3_m, pub_m]) !== 0) {
            throw "ge_frombytes_vartime returned non-zero error code";
        }
        Module.ccall("ge_scalarmult", "void", ["number", "number", "number"], [ge_p2_m, sec_m, ge_p3_m]);
        Module.ccall("ge_mul8", "void", ["number", "number"], [ge_p1p1_m, ge_p2_m]);
        Module.ccall("ge_p1p1_to_p2", "void", ["number", "number"], [ge_p2_m, ge_p1p1_m]);
        var derivation_m = Module._malloc(KEY_SIZE);
        Module.ccall("ge_tobytes", "void", ["number", "number"], [derivation_m, ge_p2_m]);
        var res = Module.HEAPU8.subarray(derivation_m, derivation_m + KEY_SIZE);
        Module._free(pub_m);
        Module._free(sec_m);
        Module._free(ge_p3_m);
        Module._free(ge_p2_m);
        Module._free(ge_p1p1_m);
        Module._free(derivation_m);
        return bintohex(res);
    };*/

    this.generate_key_derivation = function(pub, sec) {
        if (pub.length !== 64 || sec.length !== 64) {
            throw "Invalid input length";
        }
        var P = this.ge_scalarmult(pub, sec);
        return this.ge_scalarmult(P, d2s(8)); //mul8 to ensure group
    };

    this.derivation_to_scalar = function(derivation, output_index) {
        var buf = "";
        if (derivation.length !== (STRUCT_SIZES.EC_POINT * 2)) {
            throw "Invalid derivation length!";
        }
        buf += derivation;
        var enc = encode_varint(output_index);
        if (enc.length > 10 * 2) {
            throw "output_index didn't fit in 64-bit varint";
        }
        buf += enc;
        return this.hash_to_scalar(buf);
    };

    this.derive_secret_key = function(derivation, out_index, sec) {
        if (derivation.length !== 64 || sec.length !== 64) {
            throw "Invalid input length!";
        }
        var scalar_m = Module._malloc(STRUCT_SIZES.EC_SCALAR);
        var scalar_b = hextobin(this.derivation_to_scalar(derivation, out_index));
        Module.HEAPU8.set(scalar_b, scalar_m);
        var base_m = Module._malloc(KEY_SIZE);
        Module.HEAPU8.set(hextobin(sec), base_m);
        var derived_m = Module._malloc(STRUCT_SIZES.EC_SCALAR);
        Module.ccall("sc_add", "void", ["number", "number", "number"], [derived_m, base_m, scalar_m]);
        var res = Module.HEAPU8.subarray(derived_m, derived_m + STRUCT_SIZES.EC_SCALAR);
        Module._free(scalar_m);
        Module._free(base_m);
        Module._free(derived_m);
        return bintohex(res);
    };

    this.derive_public_key = function(derivation, out_index, pub) {
        if (derivation.length !== 64 || pub.length !== 64) {
            throw "Invalid input length!";
        }
        var s = this.derivation_to_scalar(derivation, out_index);
        return bintohex(nacl.ll.ge_add(hextobin(pub), hextobin(this.ge_scalarmult_base(s))));
    };

    this.hash_to_ec = function(key) {
        if (key.length !== (KEY_SIZE * 2)) {
            throw "Invalid input length";
        }
        var h_m = Module._malloc(HASH_SIZE);
        var point_m = Module._malloc(STRUCT_SIZES.GE_P2);
        var point2_m = Module._malloc(STRUCT_SIZES.GE_P1P1);
        var res_m = Module._malloc(STRUCT_SIZES.GE_P3);
        var hash = hextobin(this.cn_fast_hash(key, KEY_SIZE));
        Module.HEAPU8.set(hash, h_m);
        Module.ccall("ge_fromfe_frombytes_vartime", "void", ["number", "number"], [point_m, h_m]);
        Module.ccall("ge_mul8", "void", ["number", "number"], [point2_m, point_m]);
        Module.ccall("ge_p1p1_to_p3", "void", ["number", "number"], [res_m, point2_m]);
        var res = Module.HEAPU8.subarray(res_m, res_m + STRUCT_SIZES.GE_P3);
        Module._free(h_m);
        Module._free(point_m);
        Module._free(point2_m);
        Module._free(res_m);
        return bintohex(res);
    };


    this.generate_key_image_2 = function(pub, sec) {
        if (!pub || !sec || pub.length !== 64 || sec.length !== 64) {
            throw "Invalid input length";
        }
        var pub_m = Module._malloc(KEY_SIZE);
        var sec_m = Module._malloc(KEY_SIZE);
        Module.HEAPU8.set(hextobin(pub), pub_m);
        Module.HEAPU8.set(hextobin(sec), sec_m);
        if (Module.ccall("sc_check", "number", ["number"], [sec_m]) !== 0) {
            throw "sc_check(sec) != 0";
        }
        var point_m = Module._malloc(STRUCT_SIZES.GE_P3);
        var point2_m = Module._malloc(STRUCT_SIZES.GE_P2);
        var point_b = hextobin(this.hash_to_ec(pub));
        Module.HEAPU8.set(point_b, point_m);
        var image_m = Module._malloc(STRUCT_SIZES.KEY_IMAGE);
        Module.ccall("ge_scalarmult", "void", ["number", "number", "number"], [point2_m, sec_m, point_m]);
        Module.ccall("ge_tobytes", "void", ["number", "number"], [image_m, point2_m]);
        var res = Module.HEAPU8.subarray(image_m, image_m + STRUCT_SIZES.KEY_IMAGE);
        Module._free(pub_m);
        Module._free(sec_m);
        Module._free(point_m);
        Module._free(point2_m);
        Module._free(image_m);
        return bintohex(res);
    };

    this.generate_key_image = function(tx_pub, view_sec, spend_pub, spend_sec, output_index) {
        if (tx_pub.length !== 64) {
            throw "Invalid tx_pub length";
        }
        if (view_sec.length !== 64) {
            throw "Invalid view_sec length";
        }
        if (spend_pub.length !== 64) {
            throw "Invalid spend_pub length";
        }
        if (spend_sec.length !== 64) {
            throw "Invalid spend_sec length";
        }
        var recv_derivation = this.generate_key_derivation(tx_pub, view_sec);
        var ephemeral_pub = this.derive_public_key(recv_derivation, output_index, spend_pub);
        var ephemeral_sec = this.derive_secret_key(recv_derivation, output_index, spend_sec);
        var k_image = this.generate_key_image_2(ephemeral_pub, ephemeral_sec);
        return {
            ephemeral_pub: ephemeral_pub,
            key_image: k_image
        };
    };


    function trimRight(str, char) {
        while (str[str.length - 1] == char) str = str.slice(0, -1);
        return str;
    }

    function padLeft(str, len, char) {
        while (str.length < len) {
            str = char + str;
        }
        return str;
    }

    this.printDsts = function(dsts) {
        for (var i = 0; i < dsts.length; i++) {
            console.log(dsts[i].address + ': ' + this.formatMoneyFull(dsts[i].amount));
        }
    };

    this.formatMoneyFull = function(units) {
        units = units.toString();
        var symbol = units[0] === '-' ? '-' : '';
        if (symbol === '-') {
            units = units.slice(1);
        }
        var decimal;
        if (units.length >= config.coinUnitPlaces) {
            decimal = units.substr(units.length - config.coinUnitPlaces, config.coinUnitPlaces);
        } else {
            decimal = padLeft(units, config.coinUnitPlaces, '0');
        }
        return symbol + (units.substr(0, units.length - config.coinUnitPlaces) || '0') + '.' + decimal;
    };

    this.formatMoneyFullSymbol = function(units) {
        return this.formatMoneyFull(units) + ' ' + config.coinSymbol;
    };

    this.formatMoney = function(units) {
        var f = trimRight(this.formatMoneyFull(units), '0');
        if (f[f.length - 1] === '.') {
            return f.slice(0, f.length - 1);
        }
        return f;
    };

    this.formatMoneySymbol = function(units) {
        return this.formatMoney(units) + ' ' + config.coinSymbol;
    };

    this.parseMoney = function(str) {
        if (!str) return JSBigInt.ZERO;
        var negative = str[0] === '-';
        if (negative) {
            str = str.slice(1);
        }
        var decimalIndex = str.indexOf('.');
        if (decimalIndex == -1) {
            if (negative) {
                return JSBigInt.multiply(str, config.coinUnits).negate();
            }
            return JSBigInt.multiply(str, config.coinUnits);
        }
        if (decimalIndex + config.coinUnitPlaces + 1 < str.length) {
            str = str.substr(0, decimalIndex + config.coinUnitPlaces + 1);
        }
        if (negative) {
            return new JSBigInt(str.substr(0, decimalIndex)).exp10(config.coinUnitPlaces)
                .add(new JSBigInt(str.substr(decimalIndex + 1)).exp10(decimalIndex + config.coinUnitPlaces - str.length + 1)).negate;
        }
        return new JSBigInt(str.substr(0, decimalIndex)).exp10(config.coinUnitPlaces)
            .add(new JSBigInt(str.substr(decimalIndex + 1)).exp10(decimalIndex + config.coinUnitPlaces - str.length + 1));
    };
 
    this.is_tx_unlocked = function(unlock_time, blockchain_height) {
        if (!config.maxBlockNumber) {
            throw "Max block number is not set in config!";
        }
        if (unlock_time < config.maxBlockNumber) {
            // unlock time is block height
            return blockchain_height >= unlock_time;
        } else {
            // unlock time is timestamp
            var current_time = Math.round(new Date().getTime() / 1000);
            return current_time >= unlock_time;
        }
    };

    this.tx_locked_reason = function(unlock_time, blockchain_height) {
        if (unlock_time < config.maxBlockNumber) {
            // unlock time is block height
            var numBlocks = unlock_time - blockchain_height;
            if (numBlocks <= 0) {
                return "Transaction is unlocked";
            }
            var unlock_prediction = moment().add(numBlocks * config.avgBlockTime, 'seconds');
            return "Will be unlocked in " + numBlocks + " blocks, ~" + unlock_prediction.fromNow(true) + ", " + unlock_prediction.calendar() + "";
        } else {
            // unlock time is timestamp
            var current_time = Math.round(new Date().getTime() / 1000);
            var time_difference = unlock_time - current_time;
            if(time_difference <= 0) {
                return "Transaction is unlocked";
            }
            var unlock_moment = moment(unlock_time * 1000);
            return "Will be unlocked " + unlock_moment.fromNow() + ", " + unlock_moment.calendar();
        }
    };

    function assert(stmt, val) {
        if (!stmt) {
            throw "assert failed" + (val !== undefined ? ': ' + val : '');
        }
    }

    return this;
})(config);
