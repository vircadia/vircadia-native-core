function ready() {
    console.log("Ready");

    const electron = require('electron');
    const remote = require('remote');
    window.$ = require('./vendor/jquery/jquery-2.1.4.min.js');

    $(".state").hide();

    var currentState = null;

    function updateState(state, args) {
        if (state == 'downloading') {
            function formatBytes(size) {
                return (size / 1000000).toFixed('2');
            }
            $('#download-progress').attr('value', args.percentage * 100);
            if (args.size !== null && args.size.transferred !== null && args.size.total !== null) {
                var progressString = formatBytes(args.size.transferred) + "MB / " + formatBytes(args.size.total) + "MB";
                $('#progress-bytes').html(progressString);
            } else {
                $('#progress-bytes').html("Retrieving resources...");
            }
        } else if (state == 'installing') {
        } else if (state == 'complete') {
            setTimeout(function() {
                remote.getCurrentWindow().close();
            }, 2000);
        } else if (state == 'error') {
            $('#error-message').html(args.message);
        }

        if (currentState != state) {
            if (currentState) {
                $('#state-' + currentState).hide();
            }
            $('#state-' + state).show();
            currentState = state;
        }
    }

    electron.ipcRenderer.on('update', function(event, message) {
        updateState(message.state, message.args);
    });

    // updateState('error', { message: "This is an error message", progress: 0.5 });
    // updateState('complete', { progress: 0 });
    // updateState('downloading', { percentage: 0.5, size: { total: 83040400, transferred: 500308} });

    updateState('downloading', { percentage: 0, size: null });
    electron.ipcRenderer.send('ready');
}
