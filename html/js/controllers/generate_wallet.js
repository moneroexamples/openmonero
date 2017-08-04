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

thinwalletCtrls.controller('GenerateWalletCtrl', function ($scope, $location, AccountService) {
    "use strict";

    $scope.seed = '';
    $scope.mnemonic = '';
    $scope.mnemonic_confirmation = '';
    $scope.error = '';
    $scope.testnet = config.testnet;

    $scope.$watch('seed', function () {
        $scope.mnemonic = mn_encode($scope.seed);
        $scope.keys = cnUtil.create_address($scope.seed);
    });
    $scope.$watch('long_keys', function () {
        $scope.generateNewSeed();
    });
    $scope.generateNewSeed = function () {
        //$scope.seed = cnUtil.rand_16();
        $scope.seed = cnUtil.rand_32();
    };
    $scope.login = function (seed, mnemonic, mnemonic_confirmation) {
        $scope.error = '';
        if (mnemonic_confirmation.trim() === '') {
            $scope.error = 'Please enter a private login key';
            return;
        }
        if (mnemonic_confirmation.trim().toLocaleLowerCase() !== mnemonic.trim().toLocaleLowerCase()) {
            $scope.error = "Private login key does not match";
            return;
        }
        //var keys = cnUtil.create_address(seed);
        var keys = cnUtil.create_address(seed);

        AccountService.login(keys.public_addr, keys.view.sec, keys.spend.sec, seed, true)
            .then(function () {
                $location.path("/overview");
            }, function (reason) {
                console.error("Failed to log in: " + reason);
                $scope.error = reason;
            });
    };
});
