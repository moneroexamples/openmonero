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

thinwalletCtrls.controller('SendCoinsCtrl', function($scope, $http, $q,
                                                     AccountService, ModalService,
                                                     ApiCalls)
{
    "use strict";
    $scope.status = "";
    $scope.error = "";
    $scope.submitting = false;
    $scope.targets = [{}];
    $scope.totalAmount = JSBigInt.ZERO;
    $scope.mixins = config.defaultMixin.toString();
    $scope.view_only = AccountService.isViewOnly();

    $scope.success_page = false;
    $scope.sent_tx = {};


    var view_only = AccountService.isViewOnly();

    var explorerUrl =  config.testnet ? config.testnetExplorerUrl : config.mainnetExplorerUrl;

    // few multiplayers based on uint64_t wallet2::get_fee_multiplier
    var fee_multiplayers = [1, 4, 20, 166];

    var default_priority = 2;

    $scope.priority = default_priority.toString();

    $scope.openaliasDialog = undefined;

    function isInt(value) {
        return !isNaN(value) &&
            parseInt(Number(value)) == value &&
            !isNaN(parseInt(value, 10));
    }


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

    $scope.transferConfirmDialog = undefined;

    function confirmTransfer(address, amount, tx_hash, fee, tx_prv_key,
                             payment_id, mixin, priority, txBlobKBytes) {

        var deferred = $q.defer();

        if ($scope.transferConfirmDialog !== undefined) {
            deferred.reject("transferConfirmDialog is already being shown!");
            return;
        }

        var priority_names = ["Low", "Medium", "High", "Paranoid"];

        $scope.transferConfirmDialog = {
            address: address,
            fee: fee,
            tx_hash: tx_hash,
            amount: amount,
            tx_prv_key: tx_prv_key,
            payment_id: payment_id,
            mixin: mixin + 1,
            txBlobKBytes: Math.round(txBlobKBytes*1e3) / 1e3,
            priority: priority_names[priority - 1],
            confirm: function() {
                $scope.transferConfirmDialog = undefined;
                deferred.resolve();
            },
            cancel: function() {
                $scope.transferConfirmDialog = undefined;
                deferred.reject("Transfer canceled by user");
            }
        };
        return deferred.promise;
    }

    function getTxCharge(amount) {
        amount = new JSBigInt(amount);
        // amount * txChargeRatio
        return amount.divide(1 / config.txChargeRatio);
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

    $scope.sendCoins = function(targets, mixin, payment_id) {
        if ($scope.submitting) return;
        $scope.status = "";
        $scope.error = "";
        $scope.submitting = true;
        mixin = parseInt(mixin);
        var rct = true; //maybe want to set this later based on inputs (?)
        var realDsts = [];
        var targetPromises = [];
        for (var i = 0; i < targets.length; ++i) {
            var target = targets[i];
            if (!target.address && !target.amount) {
                continue;
            }
            var deferred = $q.defer();
            targetPromises.push(deferred.promise);
            (function(deferred, target) {
                var amount;
                try {
                    amount = cnUtil.parseMoney(target.amount);
                } catch (e) {
                    deferred.reject("Failed to parse amount (#" + i + ")");
                    return;
                }
                if (target.address.indexOf('.') === -1) {
                    try {
                        // verify that the address is valid
                        cnUtil.decode_address(target.address);
                        deferred.resolve({
                            address: target.address,
                            amount: amount
                        });
                    } catch (e) {
                        deferred.reject("Failed to decode address (#" + i + "): " + e);
                        return;
                    }
                } else {
                    var domain = target.address.replace(/@/g, ".");
                    ApiCalls.get_txt_records(domain)
                        .then(function(response) {
                            var data = response.data;
                            var records = data.records;
                            var oaRecords = [];
                            console.log(domain + ": ", data.records);
                            if (data.dnssec_used) {
                                if (data.secured) {
                                    console.log("DNSSEC validation successful");
                                } else {
                                    deferred.reject("DNSSEC validation failed for " + domain + ": " + data.dnssec_fail_reason);
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
                                deferred.reject("No OpenAlias records found for: " + domain);
                                return;
                            }
                            if (oaRecords.length !== 1) {
                                deferred.reject("Multiple addresses found for given domain: " + domain);
                                return;
                            }
                            console.log("OpenAlias record: ", oaRecords[0]);
                            var oaAddress = oaRecords[0].address;
                            try {
                                cnUtil.decode_address(oaAddress);
                                confirmOpenAliasAddress(domain, oaAddress,
                                    oaRecords[0].name, oaRecords[0].description,
                                    data.dnssec_used && data.secured)
                                    .then(function() {
                                        deferred.resolve({
                                            address: oaAddress,
                                            amount: amount,
                                            domain: domain
                                        });
                                }, function(err) {
                                    deferred.reject(err);
                                });
                            } catch (e) {
                                deferred.reject("Failed to decode OpenAlias address: " + oaRecords[0].address + ": " + e);
                                return;
                            }
                        }, function(data) {
                            deferred.reject("Failed to resolve DNS records for '" + domain + "': " + "Unknown error");
                        });
                }
            })(deferred, target);
        }

        var strpad = function(org_str, padString, length)
        {   // from http://stackoverflow.com/a/10073737/248823
            var str = org_str;
            while (str.length < length)
                str = padString + str;
            return str;
        };

        // Transaction will need at least 1KB fee (13KB for RingCT)

        var feePerKB = new JSBigInt(config.feePerKB);

        var priority = $scope.priority || default_priority;

        if (!isInt(priority))
        {
            $scope.submitting = false;
            $scope.error = "Priority is not an integer number";
            return;
        }

        if (!(priority >= 1 && priority <= 4))
        {
            $scope.submitting = false;
            $scope.error = "Priority is not between 1 and 4";
            return;
        }

        var fee_multiplayer = fee_multiplayers[priority - 1]; // default is 4

        var neededFee = rct ? feePerKB.multiply(13) : feePerKB;
        var totalAmountWithoutFee;
        var unspentOuts;
        var pid_encrypt = false; //don't encrypt payment ID unless we find an integrated one
        $q.all(targetPromises).then(function(destinations) {
            totalAmountWithoutFee = new JSBigInt(0);
            for (var i = 0; i < destinations.length; i++) {
                totalAmountWithoutFee = totalAmountWithoutFee.add(destinations[i].amount);
            }
            realDsts = destinations;
            console.log("Parsed destinations: " + JSON.stringify(realDsts));
            console.log("Total before fee: " + cnUtil.formatMoney(totalAmountWithoutFee));
            if (realDsts.length === 0) {
                $scope.submitting = false;
                $scope.error = "You need to enter a valid destination";
                return;
            }
            if (payment_id)
            {
                if (payment_id.length <= 64 && /^[0-9a-fA-F]+$/.test(payment_id))
                {
                    // if payment id is shorter, but has correct number, just
                    // pad it to required length with zeros
                    payment_id = strpad(payment_id, "0", 64);
                }

                // now double check if ok, when we padded it
                if (payment_id.length !== 64 || !(/^[0-9a-fA-F]{64}$/.test(payment_id)))
                {
                    $scope.submitting = false;
                    $scope.error = "The payment ID you've entered is not valid";
                    return;
                }

            }
            if (realDsts.length === 1) {//multiple destinations aren't supported by MyMonero, but don't include integrated ID anyway (possibly should error in the future)
                var decode_result = cnUtil.decode_address(realDsts[0].address);
                if (decode_result.intPaymentId && payment_id) {
                    $scope.submitting = false;
                    $scope.error = "Payment ID field must be blank when using an Integrated Address";
                    return;
                } else if (decode_result.intPaymentId) {
                    payment_id = decode_result.intPaymentId;
                    pid_encrypt = true; //encrypt if using an integrated address
                }
            }
            if (totalAmountWithoutFee.compare(0) <= 0) {
                $scope.submitting = false;
                $scope.error = "The amount you've entered is too low";
                return;
            }
            $scope.status = "Generating transaction...";
            console.log("Destinations: ");
            // Log destinations to console
            for (var j = 0; j < realDsts.length; j++) {
                console.log(realDsts[j].address + ": " + cnUtil.formatMoneyFull(realDsts[j].amount));
            }
            var getUnspentOutsRequest = {
                address: AccountService.getAddress(),
                view_key: AccountService.getViewKey(),
                amount: '0',
                mixin: mixin,
                // Use dust outputs only when we are using no mixins
                use_dust: mixin === 0,
                dust_threshold: config.dustThreshold.toString()
            };

            ApiCalls.get_unspent_outs(getUnspentOutsRequest)
                .then(function(request) {
                    var data = request.data;
                    unspentOuts = checkUnspentOuts(data.outputs || []);
                    unused_outs = unspentOuts.slice(0);
                    using_outs = [];
                    using_outs_amount = new JSBigInt(0);
                    if (data.per_kb_fee)
                    {
                        feePerKB = new JSBigInt(data.per_kb_fee);
                        neededFee = feePerKB.multiply(13).multiply(fee_multiplayer);
                    }
                    transfer().then(transferSuccess, transferFailure);
                }, function(data) {
                    $scope.status = "";
                    $scope.submitting = false;
                    if (data && data.Error) {
                        $scope.error = data.Error;
                        console.warn(data.Error);
                    } else {
                        $scope.error = "Something went wrong with getting your available balance for spending";
                    }
                });
        }, function(err) {
            $scope.submitting = false;
            $scope.error = err;
            console.log("Error decoding targets: " + err);
        });

        function checkUnspentOuts(outputs) {
            for (var i = 0; i < outputs.length; i++) {
                for (var j = 0; outputs[i] && j < outputs[i].spend_key_images.length; j++) {
                    var key_img = AccountService.cachedKeyImage(outputs[i].tx_pub_key, outputs[i].index);
                    if (key_img === outputs[i].spend_key_images[j]) {
                        console.log("Output was spent with key image: " + key_img + " amount: " + cnUtil.formatMoneyFull(outputs[i].amount));
                        // Remove output from list
                        outputs.splice(i, 1);
                        if (outputs[i]) {
                            j = outputs[i].spend_key_images.length;
                        }
                        i--;
                    } else {
                        console.log("Output used as mixin (" + key_img + "/" + outputs[i].spend_key_images[j] + ")");
                    }
                }
            }
            console.log("Unspent outs: " + JSON.stringify(outputs));
            return outputs;
        }

        function transferSuccess(tx_h) {
            var prevFee = neededFee;
            var raw_tx = tx_h.raw;
            var tx_hash = tx_h.hash;
            var tx_prvkey = tx_h.prvkey;
            // work out per-kb fee for transaction
            var txBlobBytes = raw_tx.length / 2;
            var txBlobKBytes = txBlobBytes / 1024.0;
            var numKB = Math.floor(txBlobKBytes);
            if (txBlobBytes % 1024) {
                numKB++;
            }
            console.log(txBlobBytes + " bytes <= " + numKB + " KB (current fee: " + cnUtil.formatMoneyFull(prevFee) + ")");
            neededFee = feePerKB.multiply(numKB).multiply(fee_multiplayer);
            // if we need a higher fee
            if (neededFee.compare(prevFee) > 0) {
                console.log("Previous fee: " + cnUtil.formatMoneyFull(prevFee) + " New fee: " + cnUtil.formatMoneyFull(neededFee));
                transfer().then(transferSuccess, transferFailure);
                return;
            }

            // generated with correct per-kb fee
            console.log("Successful tx generation, submitting tx");
            console.log("Tx hash: " + tx_hash);
            $scope.status = "Submitting...";
            var request = {
                address: AccountService.getAddress(),
                view_key: AccountService.getViewKey(),
                tx: raw_tx
            };


            confirmTransfer(realDsts[0].address, realDsts[0].amount,
                            tx_hash, neededFee, tx_prvkey, payment_id,
                            mixin, priority, txBlobKBytes).then(function() {

                //alert('Confirmed ');

                ApiCalls.submit_raw_tx(request.address, request.view_key, request.tx)
                    .then(function(response) {

                        var data = response.data;

                        if (data.status === "error")
                        {
                            $scope.status = "";
                            $scope.submitting = false;
                            $scope.error = "Something unexpected occurred when submitting your transaction: " + data.error;
                            return;
                        }

                        //console.log("Successfully submitted tx");
                        $scope.targets = [{}];
                        $scope.sent_tx = {
                            address: realDsts[0].address,
                            domain: realDsts[0].domain,
                            amount: realDsts[0].amount,
                            payment_id: payment_id,
                            tx_id: tx_hash,
                            tx_prvkey: tx_prvkey,
                            tx_fee: neededFee/*.add(getTxCharge(neededFee))*/,
                            explorerLink: explorerUrl + "tx/" + tx_hash
                        };

                        $scope.success_page = true;
                        $scope.status = "";
                        $scope.submitting = false;
                    }, function(error) {
                        $scope.status = "";
                        $scope.submitting = false;
                        $scope.error = "Something unexpected occurred when submitting your transaction: ";
                    });

            }, function(reason) {
                //alert('Failed: ' + reason);
                transferFailure("Transfer canceled");
            });


        }

        function transferFailure(reason) {
            $scope.status = "";
            $scope.submitting = false;
            $scope.error = reason;
            console.log("Transfer failed: " + reason);
        }

        var unused_outs;
        var using_outs;
        var using_outs_amount;

        function random_index(list) {
            return Math.floor(Math.random() * list.length);
        }

        function pop_random_value(list) {
            var idx = random_index(list);
            var val = list[idx];
            list.splice(idx, 1);
            return val;
        }

        function select_outputs(target_amount) {
            console.log("Selecting outputs to use. Current total: " + cnUtil.formatMoney(using_outs_amount) + " target: " + cnUtil.formatMoney(target_amount));
            while (using_outs_amount.compare(target_amount) < 0 && unused_outs.length > 0) {
                var out = pop_random_value(unused_outs);
                if (!rct && out.rct) {continue;} //skip rct outs if not creating rct tx
                using_outs.push(out);
                using_outs_amount = using_outs_amount.add(out.amount);
                console.log("Using output: " + cnUtil.formatMoney(out.amount) + " - " + JSON.stringify(out));
            }
        }

        function transfer() {
            var deferred = $q.defer();
            (function() {
                var dsts = realDsts.slice(0);
                // Calculate service charge and add to tx destinations
                //var chargeAmount = getTxCharge(neededFee);
                //dsts.push({
                //    address: config.txChargeAddress,
                //    amount: chargeAmount
                //});
                // Add fee to total amount
                var totalAmount = totalAmountWithoutFee.add(neededFee)/*.add(chargeAmount)*/;
                console.log("Balance required: " + cnUtil.formatMoneySymbol(totalAmount));

                select_outputs(totalAmount);

                //compute fee as closely as possible before hand
                if (using_outs.length > 1 && rct)
                {
                    var newNeededFee = JSBigInt(Math.ceil(cnUtil.estimateRctSize(using_outs.length, mixin, 2) / 1024)).multiply(feePerKB).multiply(fee_multiplayer);
                    totalAmount = totalAmountWithoutFee.add(newNeededFee);
                    //add outputs 1 at a time till we either have them all or can meet the fee
                    while (using_outs_amount.compare(totalAmount) < 0 && unused_outs.length > 0)
                    {
                        var out = pop_random_value(unused_outs);
                        using_outs.push(out);
                        using_outs_amount = using_outs_amount.add(out.amount);
                        console.log("Using output: " + cnUtil.formatMoney(out.amount) + " - " + JSON.stringify(out));
                        newNeededFee = JSBigInt(Math.ceil(cnUtil.estimateRctSize(using_outs.length, mixin, 2) / 1024)).multiply(feePerKB).multiply(fee_multiplayer);
                        totalAmount = totalAmountWithoutFee.add(newNeededFee);
                    }
                    console.log("New fee: " + cnUtil.formatMoneySymbol(newNeededFee) + " for " + using_outs.length + " inputs");
                    neededFee = newNeededFee;
                }

                if (using_outs_amount.compare(totalAmount) < 0)
                {
                    deferred.reject("Not enough spendable outputs / balance too low (have "
                        + cnUtil.formatMoneyFull(using_outs_amount) + " but need "
                        + cnUtil.formatMoneyFull(totalAmount)
                        + " (estimated fee " + cnUtil.formatMoneyFull(neededFee) + " included)");
                    return;
                }
                else if (using_outs_amount.compare(totalAmount) > 0)
                {
                    var changeAmount = using_outs_amount.subtract(totalAmount);

                    if (!rct)
                    {   //for rct we don't presently care about dustiness
                        //do not give ourselves change < dust threshold
                        var changeAmountDivRem = changeAmount.divRem(config.dustThreshold);
                        if (changeAmountDivRem[1].toString() !== "0") {
                            // add dusty change to fee
                            console.log("Adding change of " + cnUtil.formatMoneyFullSymbol(changeAmountDivRem[1]) + " to transaction fee (below dust threshold)");
                        }
                        if (changeAmountDivRem[0].toString() !== "0") {
                            // send non-dusty change to our address
                            var usableChange = changeAmountDivRem[0].multiply(config.dustThreshold);
                            console.log("Sending change of " + cnUtil.formatMoneySymbol(usableChange) + " to " + AccountService.getAddress());
                            dsts.push({
                                address: AccountService.getAddress(),
                                amount: usableChange
                            });
                        }
                    }
                    else
                    {
                        //add entire change for rct
                        console.log("Sending change of " + cnUtil.formatMoneySymbol(changeAmount)
                            + " to " + AccountService.getAddress());
                        dsts.push({
                            address: AccountService.getAddress(),
                            amount: changeAmount
                        });
                    }
                }
                else if (using_outs_amount.compare(totalAmount) === 0 && rct)
                {
                    //create random destination to keep 2 outputs always in case of 0 change
                    var fakeAddress = cnUtil.create_address(cnUtil.random_scalar()).public_addr;
                    console.log("Sending 0 XMR to a fake address to keep tx uniform (no change exists): " + fakeAddress);
                    dsts.push({
                        address: fakeAddress,
                        amount: 0
                    });
                }

                if (mixin > 0)
                {
                    var amounts = [];
                    for (var l = 0; l < using_outs.length; l++)
                    {
                        amounts.push(using_outs[l].rct ? "0" : using_outs[l].amount.toString());
                        //amounts.push("0");
                    }
                    var request = {
                        amounts: amounts,
                        count: mixin + 1 // Add one to mixin so we can skip real output key if necessary
                    };

                    ApiCalls.get_random_outs(request.amounts, request.count)
                        .then(function(response) {
                            var data = response.data;
                            createTx(data.amount_outs);
                        }, function(data) {
                                deferred.reject('Failed to get unspent outs');
                        });
                } else if (mixin < 0 || isNaN(mixin)) {
                    deferred.reject("Invalid mixin");
                    return;
                } else { // mixin === 0
                    createTx();
                }

                // Create & serialize transaction
                function createTx(mix_outs)
                {
                    var signed;
                    try {
                        console.log('Destinations: ');
                        cnUtil.printDsts(dsts);
                        //need to get viewkey for encrypting here, because of splitting and sorting
                        if (pid_encrypt)
                        {
                            var realDestViewKey = cnUtil.decode_address(dsts[0].address).view;
                        }

                        var splittedDsts = cnUtil.decompose_tx_destinations(dsts, rct);

                        console.log('Decomposed destinations:');

                        cnUtil.printDsts(splittedDsts);

                        signed = cnUtil.create_transaction(
                            AccountService.getPublicKeys(),
                            AccountService.getSecretKeys(),
                            splittedDsts, using_outs,
                            mix_outs, mixin, neededFee,
                            payment_id, pid_encrypt,
                            realDestViewKey, 0, rct);

                    } catch (e) {
                        deferred.reject("Failed to create transaction: " + e);
                        return;
                    }
                    console.log("signed tx: ", JSON.stringify(signed));
                    //move some stuff here to normalize rct vs non
                    var raw_tx_and_hash = {};
                    if (signed.version === 1) {
                        raw_tx_and_hash.raw = cnUtil.serialize_tx(signed);
                        raw_tx_and_hash.hash = cnUtil.cn_fast_hash(raw_tx);
                        raw_tx_and_hash.prvkey = signed.prvkey;
                    } else {
                        raw_tx_and_hash = cnUtil.serialize_rct_tx_with_hash(signed);
                    }
                    console.log("raw_tx and hash:");
                    console.log(raw_tx_and_hash);
                    deferred.resolve(raw_tx_and_hash);
                }
            })();
            return deferred.promise;
        }
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
