function ready() {
    console.log("Ready");

    const electron = require('electron');
    window.$ = require('./vendor/jquery/jquery-2.1.4.min.js');

    $(".state").hide();

    var currentState = null;

    function updateState(state, args) {
        console.log(state, args);

        if (state == 'downloading') {
            console.log("Updating progress bar");
            $('#download-progress').attr('value', args.progress * 100);
        } else if (state == 'installing') {
        } else if (state == 'complete') {
        } else if (state == 'error') {
            $('#error-message').innerHTML = args.message;
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

    updateState('downloading', { progress: 0 });
}
