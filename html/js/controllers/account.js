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

thinwalletCtrls.controller('AccountCtrl', function($scope, $rootScope, $http, $q,
                                                   $interval, AccountService,
                                                   EVENT_CODES, ApiCalls) {
    "use strict";
    $scope.loggedIn = AccountService.loggedIn;
    $scope.logout = AccountService.logout;

    $scope.balance = JSBigInt.ZERO;
    $scope.locked_balance = JSBigInt.ZERO;
    $scope.total_received = JSBigInt.ZERO;
    $scope.received_unlocked = JSBigInt.ZERO;
    $scope.total_received_unlocked = JSBigInt.ZERO;
    $scope.total_sent = JSBigInt.ZERO;
    $scope.address = AccountService.getAddress();
    $scope.view_key = $scope.viewkey = AccountService.getViewKey();
    $scope.view_only  = AccountService.isViewOnly();
    $scope.spend_key = AccountService.getSpendKey();
    $scope.mnemonic = AccountService.getMnemonic();

    $scope.nettype = config.nettype;

    $scope.transactions = [];
    $scope.blockchain_height = 0;


    // var private_view_key = AccountService.getViewKey();
    

    // decrypt_payment_id();


    $scope.tx_is_confirmed = function(tx) {
       // return ($scope.blockchain_height - tx.height) > config.txMinConfirms;
        if (!tx.coinbase)
        {
            // for regular txs, by defalut 10 blocks is required for it to
            // be confirmed/spendable
            return ($scope.blockchain_height - tx.height) > config.txMinConfirms;
        }
        else
        {
            // coinbase txs require much more blocks (default 60)
            // for it to be confirmed/spendable
            return ($scope.blockchain_height - tx.height) > config.txCoinbaseMinConfirms;
        }
    };

    $scope.tx_is_unlocked = function(tx) {
        return cnUtil.is_tx_unlocked(tx.unlock_time || 0, $scope.blockchain_height);
        //return false;
    };

    $scope.tx_is_mempool = function(tx) {
        //console.log(tx.mempool);
        return tx.mempool;
    };

    $scope.tx_locked_reason = function(tx) {
        return cnUtil.tx_locked_reason(tx.unlock_time || 0, $scope.blockchain_height);
    };

    $scope.set_locked_balance = function(locked_balance) {
        $scope.locked_balance = locked_balance;
    };
    $scope.set_total_received = function(total_received) {
        $scope.total_received = total_received;
    };
    $scope.set_total_sent = function(total_sent) {
        $scope.total_sent = total_sent;
    };

    $scope.$on(EVENT_CODES.authStatusChanged, function() {
        $scope.address = AccountService.getAddress();
        $scope.view_key = $scope.viewkey = AccountService.getViewKey();
        $scope.spend_key = AccountService.getSpendKey();
        $scope.mnemonic = AccountService.getMnemonic();
        $scope.view_only = AccountService.isViewOnly();
        if (!AccountService.loggedIn()) {
            $scope.transactions = [];
            $scope.blockchain_height = 0;
            $scope.balance = JSBigInt.ZERO;
            $scope.locked_balance = JSBigInt.ZERO;
            $scope.total_received = JSBigInt.ZERO;
            $scope.total_received_unlocked = JSBigInt.ZERO;
            $scope.received_unlocked = JSBigInt.ZERO;
            $scope.total_sent = JSBigInt.ZERO;
        }
    });

    $scope.fetchAddressInfo = function() {
        if (AccountService.loggedIn()) {
            ApiCalls.fetchAddressInfo(AccountService.getAddress(), AccountService.getViewKey())
                .then(function(response) {

                    var data = response.data;

                    var promises = [];

                    var view_only = AccountService.isViewOnly();

                    for (var i = 0; i < (data.spent_outputs || []).length; ++i)
                    {
                        var deferred = $q.defer();
                        promises.push(deferred.promise);

                        if (view_only === false)
                        {
                            (function(deferred, spent_output) {
                                setTimeout(function() {
                                    var key_image = AccountService.cachedKeyImage(
                                        spent_output.tx_pub_key,
                                        spent_output.out_index
                                    );
                                    if (spent_output.key_image !== key_image) {
                                        data.total_sent = new JSBigInt(data.total_sent).subtract(spent_output.amount);
                                    }
                                    deferred.resolve();
                                }, 0);
                            })(deferred, data.spent_outputs[i]);
                        }
                    }
                    $q.all(promises).then(function() {

                        var scanned_block_timestamp = data.scanned_block_timestamp || 0;

                        if (scanned_block_timestamp > 0)
                            scanned_block_timestamp = new Date(scanned_block_timestamp * 1000)


                        $scope.locked_balance = new JSBigInt(data.locked_funds || 0);
                        $scope.total_sent = new JSBigInt(data.total_sent || 0);
                        //$scope.account_scanned_tx_height = data.scanned_height || 0;
                        $scope.account_scanned_block_height = data.scanned_block_height || 0;
                        $scope.account_scanned_block_timestamp = scanned_block_timestamp;
                        $scope.account_scan_start_height = data.start_height || 0;
                        //$scope.transaction_height = data.transaction_height || 0;
                        $scope.blockchain_height = data.blockchain_height || 0;
                    });
            }, function(response) {
                    console.log(response)
                });
            }
    };

    $scope.fetchTransactions = function() {
        if (AccountService.loggedIn())
        {

            var view_only = AccountService.isViewOnly();

            ApiCalls.get_address_txs(AccountService.getAddress(), AccountService.getViewKey())
                .then(function(response) {

                    var data = response.data;

                    var scanned_block_timestamp = data.scanned_block_timestamp || 0;

                    if (scanned_block_timestamp > 0)
                        scanned_block_timestamp = new Date(scanned_block_timestamp * 1000)

                    $scope.account_scanned_height = data.scanned_height || 0;
                    $scope.account_scanned_block_height = data.scanned_block_height || 0;
                    $scope.account_scanned_block_timestamp = scanned_block_timestamp;
                    $scope.account_scan_start_height = data.start_height || 0;
                    //$scope.transaction_height = data.transaction_height || 0;
                    $scope.blockchain_height = data.blockchain_height || 0;


                    var transactions = data.transactions || [];

                    for (var i = 0; i < transactions.length; ++i) {
                        if ((transactions[i].spent_outputs || []).length > 0)
                        {
                            if (view_only === false)
                            {
                                for (var j = 0; j < transactions[i].spent_outputs.length; ++j)
                                {
                                    var key_image = AccountService.cachedKeyImage(
                                        transactions[i].spent_outputs[j].tx_pub_key,
                                        transactions[i].spent_outputs[j].out_index
                                    );
                                    if (transactions[i].spent_outputs[j].key_image !== key_image)
                                    {
                                        transactions[i].total_sent = new JSBigInt(transactions[i].total_sent).subtract(transactions[i].spent_outputs[j].amount).toString();
                                        transactions[i].spent_outputs.splice(j, 1);
                                        j--;
                                    }
                                }
                            }
                        }
                        //console.log(transactions[i].total_received, transactions[i].total_sent);


                        // decrypt payment_id8 which results in using
                        // integrated address
                        if (transactions[i].payment_id.length == 16) {
                            if (transactions[i].tx_pub_key) {
                                var decrypted_payment_id8
                                    = decrypt_payment_id(transactions[i].payment_id,
                                                        transactions[i].tx_pub_key,
                                                        AccountService.getViewKey());
                                //console.log("decrypted_payment_id8: " + decrypted_payment_id8);
                                transactions[i].payment_id = decrypted_payment_id8;
                            }
                        }


                        if (view_only === false)
                        {

                            if (new JSBigInt(transactions[i].total_received || 0).add(transactions[i].total_sent || 0).compare(0) <= 0)
                            {
                                transactions.splice(i, 1);
                                i--;
                                continue;
                            }


                            transactions[i].amount = new JSBigInt(transactions[i].total_received || 0)
                                .subtract(transactions[i].total_sent || 0).toString();
                        }
                        else
                        {
                            //remove tx if zero xmr recievied. probably spent only tx,
                            //but we dont have spendkey to verify this.
                            //console.log(new JSBigInt(transactions[i].total_received));
                            //console.log(new JSBigInt(transactions[i].total_received).compare(0));
                            if (new JSBigInt(transactions[i].total_received).compare(0) <= 0)
                            {
                                transactions.splice(i, 1);
                                i--;
                                continue;
                            }
                            transactions[i].amount = new JSBigInt(transactions[i].total_received).toString();

                        }

                        transactions[i].approx_float_amount = parseFloat(cnUtil.formatMoney(transactions[i].amount));
                        transactions[i].timestamp = new Date(transactions[i].timestamp * 1000);
                    }

                    transactions.sort(function(a, b)
                    {
                        return b.id - a.id; // sort by id in database

                        //var t1 = b.timestamp;
                        //var t2 = a.timestamp;

                        //return ((t1 < t2) ? -1 : ((t1 > t2) ? 1 : 0));
                    });
                    $scope.transactions = transactions;
                    $scope.total_received = new JSBigInt(data.total_received || 0);
                    $scope.total_received_unlocked = new JSBigInt(data.total_received_unlocked || 0);
                }, function(response){
                    console.log("error")
                });
        }
    };

    $scope.isAccountCatchingUp = function() {
        return ($scope.blockchain_height - $scope.account_scanned_block_height) >= 10;
    };

    $scope.$watch(
        function(scope) {
            return {
                sent: scope.total_sent,
                received: scope.total_received,
                received_unlocked: scope.total_received_unlocked
            };
        },
        function(data) {
            $scope.balance = data.received.subtract(data.sent);
            $scope.balance_unlocked = data.received_unlocked.subtract(data.sent);
        },
        true
    );

    $rootScope.$watch('account', $scope.fetchAddressInfo);
    var fetchInterval = $interval($scope.fetchAddressInfo, 10 * 1000);
    $scope.fetchAddressInfo();
    $scope.$on('$destroy', function() {
        $interval.cancel(fetchInterval);
    });
});
