/**
 * Created by mwo on 13/07/17.
 */

thinwalletServices
    .factory('ApiCalls', function($http) {
        'use strict';

        var api = {};

        api.getVersion = function() {
            return $http.post(config.apiUrl + 'get_version');
        };

        api.login = function(public_address, view_key, gen_locally) {
            return $http.post(config.apiUrl + "login", {
                    withCredentials: true,
                    address: public_address,
                    view_key: view_key,
                    create_account: true,
                    generated_locally: gen_locally
                });
        };

        api.get_address_txs = function(public_address, view_key){
            return $http.post(config.apiUrl + 'get_address_txs', {
                address: public_address,
                view_key: view_key
            })
        };

        api.fetchAddressInfo = function(public_address, view_key){
            return $http.post(config.apiUrl + 'get_address_info', {
                address: public_address,
                view_key: view_key
            })
        };

        api.import_wallet_request = function(public_address, view_key) {
            return $http.post(config.apiUrl + 'import_wallet_request', {
                address: public_address,
                view_key: view_key
            })
        };

        api.import_recent_wallet_request = function(public_address, view_key, no_blocks) {
            return $http.post(config.apiUrl + 'import_recent_wallet_request', {
                address: public_address,
                view_key: view_key,
                no_blocks_to_import: no_blocks
            })
        };

        api.get_txt_records = function(domain) {
            return $http.post(config.apiUrl + 'get_txt_records', {
                domain: domain
            })
        };

        api.get_unspent_outs = function(unspentOuts) {
            return $http.post(config.apiUrl + 'get_unspent_outs', unspentOuts);
        };

        api.submit_raw_tx = function(public_address, view_key, raw_tx) {
            return $http.post(config.apiUrl + 'submit_raw_tx', {
                address: public_address,
                view_key: view_key,
                tx: raw_tx
            })
        };

        api.get_random_outs = function(amounts, count) {
            return $http.post(config.apiUrl + 'get_random_outs', {
                amounts: amounts,
                count: count
            })
        };

        api.get_tx = function(public_address, view_key, tx_hash) {
            return $http.post(config.apiUrl + 'get_tx', {
                address: public_address,
                view_key: view_key,
                tx_hash: tx_hash
            })
        };

        return api;
    });
