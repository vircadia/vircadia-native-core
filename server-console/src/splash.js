var remote = require('electron').remote;

ready = function() {
    window.$ = require('./vendor/jquery/jquery-2.1.4.min.js');

    var userConfig = remote.getGlobal('userConfig');
    $('#suppress-splash').change(function() {
        console.log("updating");
        userConfig.set('doNotShowSplash', $(this).is(':checked'));
    });
}
