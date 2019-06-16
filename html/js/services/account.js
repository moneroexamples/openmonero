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

thinwalletServices
    .factory('AccountService', function($rootScope, $http, $q,
                                        $location, $route,
                                        EVENT_CODES, ApiCalls) {
        "use strict";
        var public_address = '';
        var account_seed = '';
        var public_keys = {};
        var private_keys = {};
        var key_images = {};
        var logged_in = false;
        var logging_in = false;
        var view_only = false;
        var account_imported = false;
        var accountService = {};
        accountService.logout = function() {
            public_address = '';
            account_seed = '';
            public_keys = {};
            private_keys = {};
            key_images = {};
            logged_in = false;
            account_imported = false;
            view_only = false;
            $rootScope.$broadcast(EVENT_CODES.authStatusChanged);
        };

        accountService.login = function(address, view_key, spend_key, seed, generated_account) {

            spend_key = spend_key || "";

            generated_account = generated_account || false;

            var deferred = $q.defer();
            logging_in = true;
            (function() {
                accountService.logout();

                view_only = (spend_key === "");
                if (!view_key || view_key.length !== 64 || (view_only ? false : spend_key.length !== 64)) {
                    return deferred.reject("invalid secret key length");
                }
                if (!cnUtil.valid_hex(view_key) || (view_only ? false : !cnUtil.valid_hex(spend_key))) {
                    return deferred.reject("invalid hex formatting");
                }
                try {
                    public_keys = cnUtil.decode_address(address);
                } catch (e) {
                    return deferred.reject("invalid address");
                }

                if (!cnUtil.is_subaddress(address))
                {
                    var expected_view_pub = cnUtil.sec_key_to_pub(view_key);
                    var expected_spend_pub;
                    if (spend_key.length === 64) {
                        expected_spend_pub = cnUtil.sec_key_to_pub(spend_key);
                    }



                    if (public_keys.view !== expected_view_pub) {
                        accountService.logout();
                        return deferred.reject("invalid view key");
                    }
                    if (!view_only && (public_keys.spend !== expected_spend_pub)) {
                        accountService.logout();
                        return deferred.reject("invalid spend key");
                    }
                }
                public_address = address;
                private_keys = {
                    view: view_key,
                    spend: spend_key
                };
                if (!!seed) {
                    var expected_account = cnUtil.create_address(seed);
                    if (expected_account.view.sec !== view_key ||
                        expected_account.spend.sec !== spend_key ||
                        expected_account.public_addr !== public_address) {
                        return deferred.reject("invalid seed");
                    }
                    account_seed = seed;
                } else {
                    account_seed = '';
                }
                logged_in = true;
                // console.log("logged_in = true;");

                ApiCalls.login(public_address, view_key, generated_account)
                    .then(function(response) {
                        // set account_imported to true if we are not logging in with a newly generated account, and a new account was created on the server

                        var data = response.data;

                        if (data.status === "error")
                        {
                            accountService.logout();
                            deferred.reject(data.reason);
                            return;
                        }

                        //console.log(data);

                        account_imported = !generated_account && data.new_address;
                        logging_in = false;
                        $rootScope.$broadcast(EVENT_CODES.authStatusChanged);
                        deferred.resolve(data);
                    }, function(reason) {
                        console.log(reason);
                        accountService.logout();
                        deferred.reject((reason || {}).Error || reason || "server error");
                    });
            })();
            deferred.promise.then(
                function() {
                    logging_in = false;
                },
                function() {
                    logging_in = false;
                }
            );
            return deferred.promise;
        };
        accountService.getAddress = function() {
            return public_address;
        };
        accountService.getAddressAndViewKey = function() {
            return {
                address: public_address,
                view_key: private_keys.view
            };
        };
        accountService.getPublicKeys = function() {
            return public_keys;
        };
        accountService.getSecretKeys = function() {
            return private_keys;
        };
        accountService.getViewKey = function() {
            return private_keys.view;
        };
        accountService.getSpendKey = function() {
            return private_keys.spend;
        };
        accountService.isViewOnly = function() {
            return view_only;
        };
        accountService.getSeed = function() {
            return account_seed;
        };
        accountService.getMnemonic = function(language) {
            return mn_encode(account_seed, language);
        };
        accountService.loggedIn = function() {
            return logged_in;
        };
        accountService.loggingIn = function() {
            return logging_in;
        };
        accountService.wasAccountImported = function() {
            return account_imported;
        };
        accountService.checkPageAuth = function() {
            if ($route.current === undefined || accountService.loggingIn()) {
                return;
            }
            if ($route.current.authenticated && !accountService.loggedIn()) {
                $location.path('/');
            }
            if ($route.current.redirectToAccount && accountService.loggedIn()) {
                $location.path('/overview');
            }
        };
        accountService.cachedKeyImage = function(tx_pub_key, out_index) {
            var cache_index = tx_pub_key + ':' + public_address + ':' + out_index;
            if (key_images[cache_index]) {
                return key_images[cache_index];
            }
            key_images[cache_index] = cnUtil.generate_key_image(
                tx_pub_key,
                private_keys.view,
                public_keys.spend,
                private_keys.spend,
                out_index
            ).key_image;
            return key_images[cache_index];
        };

        return accountService;
    });
