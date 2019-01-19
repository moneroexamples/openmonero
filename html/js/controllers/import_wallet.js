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

thinwalletCtrls.controller("ImportWalletCtrl", function($scope, $location, $http,
                                                        AccountService, ModalService,
                                                        $interval, $timeout, ApiCalls) {
    "use strict";
    $scope.payment_address = '';
    $scope.payment_id = '';
    $scope.import_fee = JSBigInt.ZERO;
    $scope.status = '';
    $scope.command = '';
    $scope.success = '';

    function get_import_request() {
        if ($scope.account_scan_start_height === 0) {
            ModalService.hide('import-wallet');
            return;
        }
        ApiCalls.import_wallet_request(AccountService.getAddress(), AccountService.getViewKey())
            .then(function(response) {

                var data = response.data;

                if ('status' in data === true && data.status == "error") {
                    $scope.status = data.error || "Some error occurred";
                    $scope.error = data.error || "Some error occurred";
                    return;
                }


                if (data.import_fee !== 0) {
                    $scope.command = 'transfer ' + data.payment_address + ' ' + cnUtil.formatMoney(data.import_fee);
                    $scope.payment_id = data.payment_id;
                    $scope.payment_address = data.payment_address;
                    $scope.import_fee = new JSBigInt(data.import_fee);
                }
                else
                {
                    $scope.command = 'import is free';
                    $scope.payment_address = 'N/A';
                    $scope.import_fee = new JSBigInt(0);
                    $scope.payment_id = "N/A";
                }

                $scope.status = data.status;

                if (data.request_fulfilled === true) {

                    //console.log(data);

                    if (data.new_request === true) {
                        if (data.import_fee !== 0) {
                            $scope.success = "Payment received. Import will start shortly. This window will close in few seconds.";
                        }
                        else {
                            $scope.success = "Import will start shortly. This window will close in few seconds.";
                        }
                    }
                    else {
                        $scope.success = "The wallet is being imported now or it has already been imported before.";
                    }

                    $timeout(function(){ModalService.hide('import-wallet')}, 5000);
                }
            },function(response) {
                var data = response.data;
                $scope.error = data && data.Error
                        ? data.Error
                        : "Something went wrong getting payment details";
            });
    }

    var getRequestInterval = $interval(get_import_request, 10 * 1000);
    get_import_request();

    $scope.$on('$destroy', function() {
        $interval.cancel(getRequestInterval);
    });

});
