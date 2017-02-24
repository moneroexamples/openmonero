thinwalletCtrls.controller('TempCtrl', function ($scope, $http) {

    $scope.last_git_commit_date = "";
    $scope.last_git_commit_hash = "";
    $scope.monero_version_full = "";

    $http.post(config.apiUrl + 'get_version')
        .success(function(data) {
            $scope.last_git_commit_date = data.last_git_commit_date;
            $scope.last_git_commit_hash = data.last_git_commit_hash;
            $scope.monero_version_full  = data.monero_version_full;

        })
        .error(function(data) {
            console.log("Error getting version. No big deal.")
        });
});