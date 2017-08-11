/**
 * Created by mwo on 02/08/17.
 */


thinwalletCtrls.controller('TransactionDetailsCtrl', function ($scope,
                                                               AccountService,
                                                               ModalService,
                                                               ApiCalls) {
    "use strict";

    // we assume that we have tx_hash in the modal's url,
    // e.g. transaction-details?tx_hash=963773fce9f1e8f285f59cdc3cc69883f43de1c4edaa7b01cc5522b48cad9631
    // so we need to extract the hash, validate it, and use to make an ajax query to the backend
    // for more detailed info about this tx.

    $scope.error = "";

    var tx_hash = ModalService.getModalUrlParams("tx_hash");

    console.log(tx_hash);

    if (tx_hash === null) {
        $scope.tx_hash = "tx_hash is not provided";
        return;
    }

    $scope.tx_hash = tx_hash;

    ApiCalls.get_tx(tx_hash)
        .then(function(response) {
            var data = response.data;
            console.log(data);

            $scope.ring_size = data.mixin_no;
            $scope.fee = data.fee;

        }, function(data) {
            $scope.error = 'Failed to get tx detailed from the backend';
        });


});
