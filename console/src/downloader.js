function ready() {
    console.log("Ready");

    const electron = require('electron');
    const remote = require('remote');
    window.$ = require('./vendor/jquery/jquery-2.1.4.min.js');

    $(".state").hide();

    var currentState = null;

    function updateState(state, args) {
        console.log(state, args);
        if (state == 'downloading') {
            $('#download-progress').attr('value', args.progress * 100);
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

    updateState('downloading', { progress: 0 });
    electron.ipcRenderer.send('ready');
}
