$(function() {
    const ipcRenderer = require('electron').ipcRenderer;
    const HFProcess = require('./modules/hf-process.js');

    function onProcessUpdate(event, arg) {
        console.log("update", event, arg);

        // var interfaceState = arg.interface.state;
        // $('#process-interface .status').text(interfaceState);
        // var interfaceOn = interfaceState != 'stopped';
        // $('#process-interface .power-on').prop('disabled', interfaceOn);
        // $('#process-interface .power-off').prop('disabled', !interfaceOn);

        var serverState = arg.home.state;
        var serverCircles = $('#ds-status .circle, #ac-status .circle');
        
        switch (serverState) {
            case HFProcess.ProcessStates.STOPPED:
                serverCircles.attr('class', 'circle stopped');
                break;
            case HFProcess.ProcessStates.STOPPING:
                serverCircles.attr('class', 'circle stopping');
                break;
            case HFProcess.ProcessStates.STARTED:
                serverCircles.attr('class', 'circle started');
                break;
        }
    }

    $('#process-interface .power-on').click(function() {
        ipcRenderer.send('start-process', { name: 'interface' });
    });
    $('#process-interface .power-off').click(function() {
        ipcRenderer.send('stop-process', { name: 'interface' });
    });
    $('#manage-server #restart').click(function() {
        ipcRenderer.send('restart-server', { name: 'home' });
    });
    $('#manage-server #stop').click(function() {
        ipcRenderer.send('stop-server', { name: 'home' });
    });
    $('#open-logs').click(function() {
        ipcRenderer.send('open-logs');
    });

    ipcRenderer.on('process-update', onProcessUpdate);

    ipcRenderer.send('update');
});
