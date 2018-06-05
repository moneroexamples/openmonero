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

thinwalletCtrls.controller("ImportAccountCtrl", function($scope, $location,
                                                         $http, AccountService,
                                                         ModalService, $timeout,
                                                         ApiCalls) {
    "use strict";

    $scope.success = '';
    $scope.no_blocks_to_import = "1000";


    // if we are import all tx from blockchain,
    // go to different modal and its controller
    $scope.importAll = function()
    {
        ModalService.show('import-wallet');
        return;
    }


    // if we import recent blocks, we can make this request here
    // as its much less complicated that importAll.
    $scope.importLast = function()
    {
        if ($scope.no_blocks_to_import > 132000) {
            ModalService.hide('imported-account');
            return;
        }

        ApiCalls.import_recent_wallet_request(AccountService.getAddress(),
                                              AccountService.getViewKey(),
                                              $scope.no_blocks_to_import)
            .then(function(response) {

                var data = response.data;

                $scope.status = data.status;

                if (data.request_fulfilled === true) {
                    $scope.success = "Request successful. Import will start shortly. This window will close in few seconds.";
                    $timeout(function(){ModalService.hide('imported-account')}, 5000);
                }
                else
                {
                    $scope.error = data.Error || "An unexpected server error occurred";
                }
            },function(err) {
                $scope.error = "An unexpected server error occurred";
            });
    }

});
