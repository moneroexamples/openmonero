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
//    materials provided wit      $interval.cancel(fetchInterval);h the distribution.
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

thinwalletCtrls.controller('TransactionsCtrl', function ($scope, $rootScope, $http, $interval, AccountService) {
    "use strict";

    $scope.pageNum = 0;
    $scope.pageSize = 20;

    $scope.predicate = 'id';
    $scope.reverse = true;

    $scope.view_only = AccountService.isViewOnly();

    $scope.predicateIcon = function(expected) {
        if ($scope.predicate === expected) {
            if($scope.reverse) {
                return "fa-caret-down";
            } else {
                return "fa-caret-up";
            }
        }
        return "";
    };

    $scope.clickedPredicate = function(name) {
        if($scope.predicate === name) {
            $scope.reverse = !$scope.reverse;
        } else {
            $scope.predicate = name;
            $scope.reverse = true;
        }
        $scope.pageNum = 0;
        tx_detail_hashes = [];
    };

    $scope.getNumPages = function () {
        return Math.ceil($scope.transactions.length / $scope.pageSize);
    };

    $scope.navigatePage = function (num) {
        var newPageNum = $scope.pageNum + num;
        if (newPageNum < 0 || newPageNum >= $scope.getNumPages()) {
            return;
        }
        $scope.pageNum = newPageNum;
        tx_detail_hashes = [];
    };

    $rootScope.$watch('account', $scope.fetchTransactions);
    var fetchInterval = $interval($scope.fetchTransactions, 10 * 1000);
    $scope.fetchTransactions();

    $scope.$on('$destroy', function () {
        $interval.cancel(fetchInterval);
    });

    var tx_detail_hashes = [];

    $scope.toggle_tx_detail = function(tx) {
        var index = tx_detail_hashes.indexOf(tx.hash);
        if(tx_detail_hashes.indexOf(tx.hash) === -1) {
            tx_detail_hashes.push(tx.hash);
        } else {
            tx_detail_hashes.splice(index, 1)
        }
    };

    $scope.showing_tx_detail = function(tx) {
        return tx_detail_hashes.indexOf(tx.hash) !== -1;
    };
});

thinwalletCtrls.filter('startFrom', function () {
    return function (input, start) {
        start = +start;
        return input.slice(start);
    }
});
