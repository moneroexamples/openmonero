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

    // check the tx_hash if it has expected format
    if (tx_hash.length !== 64 || !(/^[0-9a-fA-F]{64}$/.test(tx_hash)))
    {
        $scope.error = "Tx hash has incorrect format";
        return;
    }

    // tx_hash seems ok, so ask the backed for its details.
    ApiCalls.get_tx(tx_hash)
        .then(function(response) {
            var data = response.data;

            if (data.error) {
                $scope.error = data.status;
                return;
            }

            // set data to be shown in the modal window
            $scope.ring_size        = data.mixin_no;
            $scope.fee              = data.fee;
            $scope.tx_size          = Math.round(data.size*1e3) / 1e3;
            $scope.no_confirmations = no_confirmations;
            $scope.tx_height        = tx_height;
            $scope.tx_pub_key       = pub_key;

        }, function(data) {
            $scope.error = 'Failed to get tx detailed from the backend';
        });


});
