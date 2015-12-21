ready = function() {
    window.$ = require('./jquery');

    const ipcRenderer = require('electron').ipcRenderer;

    function onProcessUpdate(event, arg) {
        // Update interface
        console.log("update", event, arg);
        var interfaceState = arg.interface.state;
        $('#process-interface .status').text(interfaceState);
        var interfaceOn = interfaceState != 'stopped';
        $('#process-interface .power-on').prop('disabled', interfaceOn);
        $('#process-interface .power-off').prop('disabled', !interfaceOn);

        var serverState = arg.home.state;
        $('#server .status').text(serverState);
        var serverOn = serverState != 'stopped';
        $('#server .power-on').prop('disabled', serverOn);
        $('#server .power-off').prop('disabled', !serverOn);
    }

    $('#process-interface .power-on').click(function() {
        ipcRenderer.send('start-process', { name: 'interface' });
    });
    $('#process-interface .power-off').click(function() {
        ipcRenderer.send('stop-process', { name: 'interface' });
    });
    $('#server .power-on').click(function() {
        ipcRenderer.send('start-server', { name: 'home' });
    });
    $('#server .power-off').click(function() {
        ipcRenderer.send('stop-server', { name: 'home' });
    });
    $('#open-logs').click(function() {
        ipcRenderer.send('open-logs');
    });

    ipcRenderer.on('process-update', onProcessUpdate);

    ipcRenderer.send('update');
};
