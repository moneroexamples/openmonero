thinwalletDirectives.directive('showModal', function(ModalService) {
    return {
        link: function(scope, element, attrs) {
            var modal = attrs.showModal;
            element.click(function() {
                scope.$apply(function() {
                    ModalService.show(modal);
                });
            });
        }
    };
});

thinwalletDirectives.directive('hideModal', function(ModalService) {
    return {
        link: function(scope, element, attrs) {
            var modal = attrs.hideModal;
            if(!modal) {
                modal = element.parents('[modal-name]').attr('modal-name');
            }
            element.click(function() {
                scope.$apply(function() {
                    ModalService.hide(modal);
                });
            });
        }
    };
});