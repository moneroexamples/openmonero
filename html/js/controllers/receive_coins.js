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

thinwalletCtrls.controller('ReceiveCoinsCtrl', function($scope, AccountService, EVENT_CODES) {
    "use strict";
    // $scope.accountService = AccountService;
    
    // $scope.params = {
    //     address: $scope.address
    // };
    // // Generate random payment id
    // $scope.payment_id = cnUtil.rand_32(); 

    $scope.error = "";   


    var strpad = function(org_str, padString, length)
    {   // from http://stackoverflow.com/a/10073737/248823
        var str = org_str;
        while (str.length < length)
            str = padString + str;
        return str;
    };

    var public_address = AccountService.getAddress(); 

    $scope.payment_id8 = rand_8(); // random 8 byte payment id
    $scope.payment_id8_rendered = $scope.payment_id8;

    $scope.integarted_address = get_account_integrated_address(
        public_address, $scope.payment_id8);

    $scope.$watch("payment_id8", function (newValue, oldValue) {
            
            if (oldValue !== newValue) 
            {

                var payment_id8 = $scope.payment_id8;


                if (payment_id8.length <= 16 && /^[0-9a-fA-F]+$/.test(payment_id8))                 {
                    // if payment id is shorter, but has correct number, just
                    // pad it to required length with zeros
                    payment_id8 = strpad(payment_id8, "0", 16);
                }

                // now double check if ok, when we padded it
                if (payment_id8.length !== 16 || !(/^[0-9a-fA-F]{16}$/.test(payment_id8)))
                {
                    $scope.integarted_address = "Not avaliable due to error"
                    $scope.error = "The payment ID you've entered is not valid";
                    return;
                }

                $scope.integarted_address = get_account_integrated_address(
                    public_address, payment_id8);

                $scope.payment_id8_rendered = payment_id8;

                $scope.error = "";
            }
        }, 
        true
    );

    $scope.new_payment_id8 = function()
    {
        $scope.payment_id8 = rand_8();                
    }

    //
    //var payment_id8 = rand_8();        
    //var integarted_address = get_account_integrated_address(keys.public_addr, payment_id8);

    // $scope.$watch(
    //     function(scope) {
    //         return {
    //             address: scope.address,
    //             label: scope.label,
    //             payment_id: scope.payment_id,
    //             message: scope.message,
    //             amount: scope.amount
    //         };
    //     },
    //     function(data) {
    //         $scope.params = data;
    //     },
    //     true
    // );
});
