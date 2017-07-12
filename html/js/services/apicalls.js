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


        return api;
    });