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

thinwalletServices
    .factory('ModalService', function () {
        "use strict";
        var modalService = {};
        var currentModal = '';

        modalService.show = function (modal_name) {
            //console.log("Showing modal: " + modal_name);
            currentModal = modal_name;
        };

        modalService.hide = function (modal_name) {
            //console.log("Hiding modal: " + modal_name + ", " + currentModal);

            if (!modal_name || currentModal.split("?")[0] === modal_name) {
                currentModal = '';
            }
        };

        modalService.getModal = function () {
            return currentModal;
        };

        modalService.getModalURL = function () {
            if (!currentModal) return '';
            var split_by_param = currentModal.split("?");

            // we may pass parameters in model window name, e.g.,
            // transaction-details?tx_hash=963773fce9f1e8f285f59cdc3cc69883f43de1c4edaa7b01cc5522b48cad9631
            var modal_url = "modals/" + split_by_param[0] + ".html?1";

            return modal_url;
        };

        modalService.getModalUrlParams = function(param_to_get) {

            // the "http://127.0.0.1/" is dummy below, only used
            // so that URL does not complain
            var url_parsed = new URL("http://127.0.0.1/" + currentModal);
            return url_parsed.searchParams.get(param_to_get);
        };


        return modalService;
    });
