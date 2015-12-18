ready = function() {
    window.$ = require('./vendor/jquery/jquery-2.1.4.min.js');

    const ipcRenderer = require('electron').ipcRenderer;

    function onProcessUpdate(event, arg) {
        // Update interface
        console.log("update", event, arg);
        var state = arg.interface.state;
        $('#process-interface .status').text(state);
        var on = state != 'stopped';
        if (on) {
            $('#process-interface .power-on').hide();
            $('#process-interface .power-off').show();
        } else {
            $('#process-interface .power-on').show();
            $('#process-interface .power-off').hide();
        }
    }

    $('#process-interface .power-on').click(function() {
        ipcRenderer.send('start-process', { name: 'interface' });
    });
    $('#process-interface .power-off').click(function() {
        ipcRenderer.send('stop-process', { name: 'interface' });
    });

    ipcRenderer.on('process-update', onProcessUpdate);

    ipcRenderer.send('update');
};
