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

    $scope.mnemonic_language = 'english';

    $scope.login_mnemonic = function(mnemonic, language) {
        $scope.error = '';
        var seed;
        var keys;
        mnemonic = mnemonic.toLowerCase() || "";
        try {
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
            }, function(reason) {
                $scope.error = reason;
                console.error(reason);
            });
    };

    $scope.login_keys = function(address, view_key, spend_key) {
        $scope.error = '';
        AccountService.login(address, view_key, spend_key, undefined, false)
            .then(function() {
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
