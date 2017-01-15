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

thinwalletCtrls.controller('VanityAddressCtrl', function ($scope) {
    "use strict";
    $scope.running = false;
    $scope.num_searched = 0;

    function score(address, prefix) {
        address = address.toUpperCase();
        for (var i = 0; i < prefix.length; ++i) {
            if (address[i] !== prefix[i] && prefix[i] !== '*') return i;
        }
        return prefix.length;
    }

    function startGeneration(prefix) {
        $scope.found = {};
        $scope.num_searched = 0;
        var num_searched = 0;
        var seed = cnUtil.rand_16();
        var bestMatch = 0;
        var last_generated = 0;
        prefix = prefix.toUpperCase();

        function generateAddress() {
            for (var i = 0; i < 10; ++i) {
                if (!$scope.running) return;
                var address = cnUtil.create_addr_prefix(seed);
                ++num_searched;
                var sc = score(address, prefix);
                if (sc === prefix.length) {
                    $scope.found = {
                        mnemonic: mn_encode(seed),
                        address: cnUtil.create_address(seed).public_addr
                    };
                    $scope.num_searched = num_searched;
                    $scope.running = false;
                    break;
                } else {
                    if (sc >= bestMatch) {
                        bestMatch = sc;
                        $scope.found = {
                            mnemonic: mn_encode(seed),
                            address: cnUtil.create_address(seed).public_addr
                        };
                    }
                }
                var lastByte;
                lastByte = parseInt(seed.slice(30, 32), 16);
                if (lastByte < 0xff) {
                    seed = seed.slice(0, 30) + ('0' + (lastByte + 1).toString(16)).slice(-2);
                } else {
                    seed = cnUtil.keccak(seed, 16, 32).slice(0, 30) + '00';
                }
            }
            if (num_searched - last_generated > 10000) {
                seed = cnUtil.rand_16();
                last_generated = num_searched;
            }
            $scope.num_searched = num_searched;
            $scope.$apply();
            setTimeout(generateAddress, 0);
        }

        setTimeout(generateAddress, 0);
    }

    $scope.$on('$destroy', function () {
        $scope.running = false;
    });

    $scope.toggleGeneration = function () {
        $scope.running = !$scope.running;
        if ($scope.running) {
            if (!$scope.prefix || $scope.prefix[0] !== '4') {
                $scope.running = false;
                return;
            }
            startGeneration($scope.prefix);
        }
    };
});
