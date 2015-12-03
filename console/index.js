ready = function() {
    window.$ = require('./jquery');

    const ipcRenderer = require('electron').ipcRenderer;

    function onProcessUpdate(event, arg) {
        // Update interface
        console.log("update", event, arg);
        var state = arg.interface.state;
        $('#process-interface .status').text(state);
        $('#process-interface .power').prop('disabled', state != 'stopped');
    }

    $('#process-interface .power').click(function() {
        ipcRenderer.send('start-process', { name: 'interface' });
    });

    ipcRenderer.on('process-update', onProcessUpdate);
};
