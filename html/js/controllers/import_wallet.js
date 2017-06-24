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

thinwalletCtrls.controller("ImportWalletCtrl", function($scope, $location, $http, AccountService, ModalService, $interval) {
    "use strict";
    $scope.payment_address = '';
    $scope.payment_id = '';
    $scope.import_fee = JSBigInt.ZERO;
    $scope.status = '';
    $scope.command = '';

    function get_import_request() {
        if ($scope.account_scan_start_height === 0) {
            ModalService.hide('import-wallet');
            return;
        }
        $http.post(config.apiUrl + "import_wallet_request", {
            address: AccountService.getAddress(),
            view_key: AccountService.getViewKey()
        }).success(function(data) {
            $scope.command = 'transfer ' + data.payment_address + ' ' + cnUtil.formatMoney(data.import_fee) + ' ' + data.payment_id;
            $scope.payment_id = data.payment_id;
            $scope.payment_address = data.payment_address;
            $scope.import_fee = new JSBigInt(data.import_fee);
            $scope.status = data.status;
            if (data.request_fulfilled) {
                ModalService.hide('import-wallet');
            }
        }).error(function(err) {
            $scope.error = err.Error || err || "An unexpected server error occurred";
        });
    }

    var getRequestInterval = $interval(get_import_request, 10 * 1000);
    get_import_request();

    $scope.$on('$destroy', function() {
        $interval.cancel(getRequestInterval);
    });

    $scope.importLast = function(no_blocks) {
        alert(no_blocks);
    }

});
