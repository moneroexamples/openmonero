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

    var view_only = AccountService.isViewOnly();


    $scope.tx_hash = tx_hash;

    // check the tx_hash if it has expected format
    if (tx_hash.length !== 64 || !(/^[0-9a-fA-F]{64}$/.test(tx_hash)))
    {
        $scope.error = "Tx hash has incorrect format";
        return;
    }

    var address = AccountService.getAddress();
    var view_key = AccountService.getViewKey();

    // tx_hash seems ok, so ask the backed for its details.
    ApiCalls.get_tx(address, view_key, tx_hash)
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
            $scope.no_confirmations = data.no_confirmations;
            $scope.tx_height        = data.tx_height;
            $scope.tx_pub_key       = data.pub_key;

            var total_received      = data.total_received;
            var total_sent          = data.total_sent;

            if ((data.spent_outputs || []).length > 0)
            {
                if (view_only === false)
                {
                    for (var j = 0; j < data.spent_outputs.length; ++j)
                    {
                        var key_image = AccountService.cachedKeyImage(
                            data.spent_outputs[j].tx_pub_key,
                            data.spent_outputs[j].out_index
                        );
                        if (data.spent_outputs[j].key_image !== key_image)
                        {
                            total_sent = new JSBigInt(total_sent).subtract(data.spent_outputs[j].amount);
                            data.spent_outputs.splice(j, 1);
                            j--;
                        }
                    }
                }
            }

            $scope.tx_amount = new JSBigInt(total_received || 0).subtract(total_sent || 0).toString();

            console.log($scope.tx_amount);

        }, function(data) {
            $scope.error = 'Failed to get tx detailed from the backend';
        });


});
