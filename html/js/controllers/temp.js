thinwalletCtrls.controller('TempCtrl', function ($scope, $http) {

    $scope.last_git_commit_date = "";
    $scope.last_git_commit_hash = "";
    $scope.monero_version_full = "";

    $http.post(config.apiUrl + 'get_version')
        .success(function(data) {
            last_git_commit_date = data.last_git_commit_date;
            last_git_commit_hash = data.last_git_commit_hash;
            monero_version_full  = data.monero_version_full;

            $scope.version = "Open Monero version: "
                + data.last_git_commit_date + "-" + data.last_git_commit_hash
                + "  | Monero version: " + data.monero_version_full
                + "  | Blockchain height: " + data.blockchain_height

        })
        .error(function(data) {
            //console.log("Error getting version. No big deal.")
            $scope.version = "Error: Can't connect to the backend! Maybe its down."
        });
});