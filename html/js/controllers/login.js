// Copyright (c) 2014-2015, MyMonero.com
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

thinwalletCtrls.controller("LoginCtrl", function($scope, $location, AccountService, ModalService) {
    "use strict";
    $scope.tab_number = 1;

    $scope.set_tab = function(number) {
        $scope.tab_number = number;
        $scope.error = '';
    };

    $scope.decode = false;

    $scope.mnemonic_language = 'english';

    if (config.nettype == 1) {
        // just some dummy account, as not to fill login form every time.
        $scope.mnemonic = "agenda shrugged liquid extra mundane phone nomad oust duckling sifting pledge loyal royal urban skater bawled gusts bounced boil violin mumble gags axle sapling shrugged";
    } else if (config.nettype == 2) {
        // just some dummy account, as not to fill login form every time.
        $scope.mnemonic = "peaches purged gossip either gyrate organs asked ability autumn inexact coffee rays avidly fountain foxes wrist goldfish masterful anecdote sulking masterful science beyond coffee coffee";
    } else {
        $scope.address = "44AFFq5kSiGBoZ4NMDwYtN18obc8AemS33DBLWs3H7otXft3XjrpDtQGv7SqSsaBYBb98uNbr2VBBEt7f2wfn3RVGQBEP3A";
        $scope.view_key = "f359631075708155cc3d92a32b75a7d02a5dcf27756707b47a2b31b21c389501" ;
    }


    var decode_seed = function(mnemonic, language)
    {
        var seed;
        var keys;

        switch (language) {
            case 'english':
                try {
                    seed = mn_decode(mnemonic);
                } catch (e) {
                    // Try decoding as an electrum seed, on failure throw the original exception
                    try {
                        seed = mn_decode(mnemonic, "electrum");
                    } catch (ee) {
                        throw e;
                    }
                }
                break;
            default:
                seed = mn_decode(mnemonic, language);
                break;
        }

        keys = cnUtil.create_address(seed);


        //var payment_id8 = rand_8();        
        //var integarted_address = get_account_integrated_address(keys.public_addr, payment_id8);

        return [seed, keys];
    };


    $scope.login_mnemonic = function(mnemonic, language) {
        $scope.error = '';
        var seed;
        var keys;
        mnemonic = mnemonic.toLowerCase() || "";

        try {
            var seed_key = decode_seed(mnemonic, language);
            seed = seed_key[0];
            keys  = seed_key[1];
        } catch (e) {
            console.log("Invalid mnemonic!");
            $scope.error = e;
            return;
        }



        AccountService.login(keys.public_addr, keys.view.sec, keys.spend.sec, seed, false)
            .then(function() {
                ModalService.hide('login');
                $location.path("/overview");
                if (AccountService.wasAccountImported()) {
                    ModalService.show('imported-account');
                }
            }, function(response) {
                $scope.error = "Error connecting to the backend. Can't login.";
                //console.error(response);
            });
    };



    $scope.decodeSeed = function(mnemonic, language)
    {
        $scope.error = '';
        var seed;
        var keys;

        mnemonic = mnemonic.toLowerCase() || "";

        try
        {
            var seed_key = decode_seed(mnemonic, language);

            seed = seed_key[0];
            keys  = seed_key[1];

            $scope.address = keys.public_addr;
            $scope.view_key = keys.view.sec;
            $scope.spend_key = keys.spend.sec;

        } catch (e) {
            console.log("Invalid mnemonic!");
            $scope.error = e;
            return;
        }
    };

    $scope.login_keys = function(address, view_key, spend_key) {
        $scope.error = '';

        AccountService.login(address, view_key, spend_key, undefined, false)
            .then(function(data)
            {
                if (data.status === "error")
                {
                    $scope.status = "";
                    $scope.submitting = false;
                    $scope.error = "Something unexpected occurred when submitting your transaction: " + data.error;
                    return;
                }

                ModalService.hide('login');
                $location.path("/overview");
                if (AccountService.wasAccountImported()) {
                    ModalService.show('imported-account');
                }
            }, function(reason) {
                $scope.error = reason;
                console.error(reason);
            });
    };
});
