// Copyright (c) 2014-2017, MyMonero.com
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

class HostedMoneroAPIClient
{
    constructor(options)
    {
        const self = this
        self.$http = options.$http
        if (self.$http == null || typeof self.$http === 'undefined') {
            throw "self.$http required"
        }
    }
    //
    // Getting outputs for sending funds
    UnspentOuts(
        address,
        view_key__private,
        spend_key__public,
        spend_key__private,
        mixinNumber,
        sweeping,
        fn
    ) { // -> RequestHandle
        const self = this
        mixinNumber = parseInt(mixinNumber) // jic
        //
        var parameters =
        {
            address: address,
            view_key: view_key__private
        };
        parameters.amount = '0'
        parameters.mixin = mixinNumber
        parameters.use_dust = true // Client now filters unmixable by dustthreshold amount (unless sweeping) + non-rct 
        parameters.dust_threshold = mymonero_core_js.monero_config.dustThreshold.toString()
        const endpointPath = 'get_unspent_outs'
        self.$http.post(config.apiUrl + endpointPath, parameters).then(
            function(data)
            {
                __proceedTo_parseAndCallBack(data.data)
            }
        ).catch(
            function(data)
            {
                fn(data && data.Error ? data.Error : "Something went wrong with getting your available balance for spending");
            }
        );
        function __proceedTo_parseAndCallBack(data)
        {
            mymonero_core_js.monero_utils_promise.then(function(monero_utils)
            {
                mymonero_core_js.api_response_parser_utils.Parsed_UnspentOuts__keyImageManaged(
                    data,
                    address,
                    view_key__private,
                    spend_key__public,
                    spend_key__private,
                    monero_utils,
                    function(err, retVals)
                    {
                        if (err) {
                            fn(err)
                            return
                        }
                        fn(null, retVals.unspentOutputs, retVals.per_byte_fee__string)
                    }
                )
            }).catch(function(err)
            {
                fn(err)
            })
        }
        const requestHandle = 
        {
            abort: function()
            {
                console.warn("TODO: abort!")
            }
        }
        return requestHandle
    }
    //
    RandomOuts(
        using_outs,
        mixinNumber,
        fn
    ) { // -> RequestHandle
        const self = this
        //
        mixinNumber = parseInt(mixinNumber)
        if (mixinNumber < 0 || isNaN(mixinNumber)) {
            const errStr = "Invalid mixin - must be >= 0"
            const err = new Error(errStr)
            fn(err)
            return
        }
        //
        var amounts = [];
        for (var l = 0; l < using_outs.length; l++) {
            amounts.push(using_outs[l].rct ? "0" : using_outs[l].amount.toString())
        }
        //
        var parameters =
        {
            amounts: amounts,
            count: mixinNumber + 1 // Add one to mixin so we can skip real output key if necessary
        }
        const endpointPath = 'get_random_outs'
        self.$http.post(config.apiUrl + endpointPath, parameters).then(
            function(data)
            {
                __proceedTo_parseAndCallBack(data.data)
            }
        ).catch(
            function(data)
            {
                fn(data && data.Error ? data.Error : "Something went wrong while getting decoy outputs");
            }
        );
        function __proceedTo_parseAndCallBack(data)
        {
            console.log("debug: info: random outs: data", data)
            const amount_outs = data.amount_outs
            //
            fn(null, amount_outs)
        }
        const requestHandle = 
        {
            abort: function()
            {
                console.warn("TODO: abort!")
            }
        }
        return requestHandle
    }
    //
    // Runtime - Imperatives - Public - Sending funds
    SubmitSerializedSignedTransaction(
        address,
        view_key__private,
        serializedSignedTx,
        fn // (err?) -> RequestHandle
    ) {
        const self = this
        //
        var parameters = 
        {
            address: address,
            view_key: view_key__private
        };
        parameters.tx = serializedSignedTx
        const endpointPath = 'submit_raw_tx'
        self.$http.post(config.apiUrl + endpointPath, parameters).then(
            function(data)
            {
                if (data.data.error)
                {
                    const errStr = "Invalid mixin - must be >= 0";
                    const err = new Error(data.data.error);
                    fn(err);
                    return;
                }
                __proceedTo_parseAndCallBack(data.data)
            }
        ).catch(
            function(data)
            {
                fn(data && data.Error ? data.Error : "Something went wrong with getting your available balance for spending");
            }
        );
        function __proceedTo_parseAndCallBack(data)
        {
            console.log("debug: info: submit_raw_tx: data", data)
            fn(null)
        }
        const requestHandle = 
        {
            abort: function()
            {
                console.warn("TODO: abort!")
            }
        }
        return requestHandle
    }
}

thinwalletCtrls.controller('SendCoinsCtrl', function($scope, $http, $q, AccountService) {
    "use strict";
    $scope.status = "";
    $scope.error = "";
    $scope.submitting = false;
    $scope.targets = [{}];
    $scope.totalAmount = JSBigInt.ZERO;

    $scope.success_page = false;
    $scope.sent_tx = {};

    $scope.openaliasDialog = undefined;

    function confirmOpenAliasAddress(domain, address, name, description, dnssec_used) {
        var deferred = $q.defer();
        if ($scope.openaliasDialog !== undefined) {
            deferred.reject("OpenAlias confirm dialog is already being shown!");
            return;
        }
        $scope.openaliasDialog = {
            address: address,
            domain: domain,
            name: name,
            description: description,
            dnssec_used: dnssec_used,
            confirm: function() {
                $scope.openaliasDialog = undefined;
                console.log("User confirmed OpenAlias resolution for " + domain + " to " + address);
                deferred.resolve();
            },
            cancel: function() {
                $scope.openaliasDialog = undefined;
                console.log("User rejected OpenAlias resolution for " + domain + " to " + address);
                deferred.reject("OpenAlias resolution rejected by user");
            }
        };
        return deferred.promise;
    }


    $scope.removeTarget = function(index) {
        $scope.targets.splice(index, 1);
    };

    $scope.$watch('targets', function() {
        var totalAmount = JSBigInt.ZERO;
        for (var i = 0; i < $scope.targets.length; ++i) {
            try {
                var amount = cnUtil.parseMoney($scope.targets[i].amount);
                totalAmount = totalAmount.add(amount);
            } catch (e) {
            }
        }
        $scope.totalAmount = totalAmount;
    }, true);

    $scope.resetSuccessPage = function() {
        $scope.success_page = false;
        $scope.sent_tx = {};
    };


    $scope.sendCoins = function(targets, payment_id) 
    {
        if ($scope.submitting) {
            console.warn("Already submitting")
            return;
        }
        $scope.status = "";
        $scope.error = "";
        $scope.submitting = true;
        //
        mymonero_core_js.monero_utils_promise.then(function(monero_utils)
        {
            if (targets.length > 1) {
                throw "MyMonero currently only supports one target"
            }
            const target = targets[0]
            if (!target.address && !target.amount) {
                throw "Target address and amount required.";
            }
            function fn(err, mockedTransaction, optl_domain)
            { // this function handles all termination of sendCoins(), except for canceled_fn()
                if (err) {
                    console.error("Err:", err)
                    $scope.status = "";
                    $scope.error = "Something unexpected occurred when submitting your transaction: " + (err.Error || err);
                    $scope.submitting = false;
                    return
                }
                console.log("Tx succeeded", mockedTransaction)
                $scope.targets = [{}];
                $scope.sent_tx = {
                    address: mockedTransaction.target_address,
                    domain: optl_domain,
                    amount: mockedTransaction.total_sent, // it's expecting a JSBigInt back so it can `| money`
                    payment_id: mockedTransaction.payment_id,
                    tx_id: mockedTransaction.hash,
                    tx_fee: mockedTransaction.tx_fee,
                    tx_key: mockedTransaction.tx_key
                };
                $scope.success_page = true;
                $scope.status = "";
                $scope.submitting = false;
            }
            function canceled_fn()
            {
                $scope.status = "Canceled";
                $scope.submitting = false;
            }
            var sweeping = targets[0].sendAll === true ? true : false; // default false, i.e. non-existence of UI element to set the flag
            var amount = sweeping ? 0 : target.amount;
            if (target.address.indexOf('.') !== -1) {
                var domain = target.address.replace(/@/g, ".");
                $http.post(config.apiUrl + "get_txt_records", {
                    domain: domain
                }).then(function(data) {
                    var records = data.records;
                    var oaRecords = [];
                    console.log(domain + ": ", data.records);
                    if (data.dnssec_used) {
                        if (data.secured) {
                            console.log("DNSSEC validation successful");
                        } else {
                            fn("DNSSEC validation failed for " + domain + ": " + data.dnssec_fail_reason, null);
                            return;
                        }
                    } else {
                        console.log("DNSSEC Not used");
                    }
                    for (var i = 0; i < records.length; i++) {
                        var record = records[i];
                        if (record.slice(0, 4 + config.openAliasPrefix.length + 1) !== "oa1:" + config.openAliasPrefix + " ") {
                            continue;
                        }
                        console.log("Found OpenAlias record: " + record);
                        oaRecords.push(parseOpenAliasRecord(record));
                    }
                    if (oaRecords.length === 0) {
                        fn("No OpenAlias records found for: " + domain);
                        return;
                    }
                    if (oaRecords.length !== 1) {
                        fn("Multiple addresses found for given domain: " + domain);
                        return;
                    }
                    console.log("OpenAlias record: ", oaRecords[0]);
                    var oaAddress = oaRecords[0].address;
                    try {
                        monero_utils.decode_address(oaAddress, config.nettype);
                        confirmOpenAliasAddress(
                            domain, 
                            oaAddress, 
                            oaRecords[0].name, 
                            oaRecords[0].description, 
                            data.dnssec_used && data.secured
                        ).then(
                            function()
                            {
                                sendTo(oaAddress, amount, domain);
                            }, 
                            function(err)
                            {
                                fn(err);
                                return;
                            }
                        );
                    } catch (e) {
                        fn("Failed to decode OpenAlias address: " + oaRecords[0].address + ": " + e);
                        return;
                    }
                }).catch(function(data) {
                    fn("Failed to resolve DNS records for '" + domain + "': " + ((data || {}).Error || data || "Unknown error"));
                    return
                });
                return
            }
            // xmr address (incl subaddresses):
            try {
                // verify that the address is valid
                monero_utils.decode_address(target.address, config.nettype);
                sendTo(target.address, amount, null /*domain*/);
            } catch (e) {
                fn("Failed to decode address (#" + i + "): " + e);
                return;
            }
            //
            function sendTo(target_address, amount, domain/*may be null*/)
            {
                const mixin = 10; // mandatory fixed mixin for v8 Monero fork
                let statusUpdate_messageBase = sweeping ? `Sending wallet balance…` : `Sending ${amount} XMR…`
                function _configureWith_statusUpdate(str, code)
                {
                    console.log("Step:", str)
                    $scope.status = str
                }
                //
                _configureWith_statusUpdate(statusUpdate_messageBase);
                //
                const monero_sendingFunds_utils = mymonero_core_js.monero_sendingFunds_utils
                monero_sendingFunds_utils.SendFunds(
                    target_address,
                    config.nettype,
                    amount,
                    sweeping,
                    AccountService.getAddress(),
                    AccountService.getSecretKeys(),
                    AccountService.getPublicKeys(),
                    new HostedMoneroAPIClient({ $http: $http }),
                    payment_id, // passed in
                    1, /* priority */
                    function(code) 
                    {
                        let suffix = monero_sendingFunds_utils.SendFunds_ProcessStep_MessageSuffix[code]
                        _configureWith_statusUpdate(
                            statusUpdate_messageBase + " " + suffix, // TODO: localize this concatenation
                            code
                        )
                    },
                    function(
                        currencyReady_targetDescription_address,
                        sentAmount, final__payment_id,
                        tx_hash, tx_fee, tx_key, mixin
                    ) {
                        let total_sent__JSBigInt = sentAmount.add(tx_fee)
                        let total_sent__atomicUnitString = total_sent__JSBigInt.toString()
                        let total_sent__floatString = mymonero_core_js.monero_amount_format_utils.formatMoney(total_sent__JSBigInt) 
                        let total_sent__float = parseFloat(total_sent__floatString)
                        //
                        const mockedTransaction = 
                        {
                            hash: tx_hash,
                            mixin: "" + mixin,
                            coinbase: false,
                            mempool: true, // is that correct?
                            //
                            isJustSentTransaction: true, // this is set back to false once the server reports the tx's existence
                            timestamp: new Date(), // faking
                            //
                            unlock_time: 0,
                            //
                            // height: null, // mocking the initial value -not- to exist (rather than to erroneously be 0) so that isconfirmed -> false
                            //
                            total_sent: new JSBigInt(total_sent__atomicUnitString),
                            total_received: new JSBigInt("0"),
                            //
                            approx_float_amount: -1 * total_sent__float, // -1 cause it's outgoing
                            // amount: new JSBigInt(sentAmount), // not really used (note if you uncomment, import JSBigInt)
                            //
                            payment_id: final__payment_id, // b/c `payment_id` may be nil of short pid was used to fabricate an integrated address
                            //
                            // info we can only preserve locally
                            tx_fee: tx_fee,
                            tx_key: tx_key,
                            target_address: target_address, // only we here are saying it's the target
                        };
                        fn(null, mockedTransaction, domain)
                    },
                    function(err)
                    { // failed-fn 
                        fn(err)
                    }
                )
            }
        })
    };
});

function parseOpenAliasRecord(record) {
    var parsed = {};
    if (record.slice(0, 4 + config.openAliasPrefix.length + 1) !== "oa1:" + config.openAliasPrefix + " ") {
        throw "Invalid OpenAlias prefix";
    }
    function parse_param(name) {
        var pos = record.indexOf(name + "=");
        if (pos === -1) {
            // Record does not contain param
            return undefined;
        }
        pos += name.length + 1;
        var pos2 = record.indexOf(";", pos);
        return record.substr(pos, pos2 - pos);
    }

    parsed.address = parse_param('recipient_address');
    parsed.name = parse_param('recipient_name');
    parsed.description = parse_param('tx_description');
    return parsed;
}
