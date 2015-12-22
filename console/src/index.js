$(function() {
    const ipcRenderer = require('electron').ipcRenderer;
    const HFProcess = require('./modules/hf-process.js');

    var settingsButton = $('#manage-server #settings');

    function toggleManageButton(button, enabled) {
        if (enabled) {
            button.attr('href', '#');
            button.removeClass('disabled');
        } else {
            button.removeAttr('href');
            button.addClass('disabled');
        }
    }

    function onProcessUpdate(event, arg) {
        console.log("update", event, arg);

        // var interfaceState = arg.interface.state;
        // $('#process-interface .status').text(interfaceState);
        // var interfaceOn = interfaceState != 'stopped';
        // $('#process-interface .power-on').prop('disabled', interfaceOn);
        // $('#process-interface .power-off').prop('disabled', !interfaceOn);

        var sendingProcess = arg;

        var processCircle = null;
        if (sendingProcess.name == "domain-server") {
            processCircle = $('#ds-status .circle');
        } else if (sendingProcess.name == "ac-monitor") {
            processCircle = $('#ac-status .circle');
        } else {
            return;
        }

        if (sendingProcess.name == "domain-server") {
            toggleManageButton(settingsButton,
                               sendingProcess.state == HFProcess.ProcessStates.STARTED);
        }

        switch (sendingProcess.state) {
            case HFProcess.ProcessStates.STOPPED:
                processCircle.attr('class', 'circle stopped');
                break;
            case HFProcess.ProcessStates.STOPPING:
                processCircle.attr('class', 'circle stopping');
                break;
            case HFProcess.ProcessStates.STARTED:
                processCircle.attr('class', 'circle started');
                break;
        }
    }

    function onProcessGroupUpdate(event, arg) {
        var sendingGroup = arg;
        var stopButton = $('#manage-server #stop');
        var goButton = $('#go-server-button');

        switch (sendingGroup.state) {
            case HFProcess.ProcessGroupStates.STOPPED:
            case HFProcess.ProcessGroupStates.STOPPING:
                // if the process group is stopping, the stop button should be disabled
                toggleManageButton(stopButton, false);

                // disable the go button
                goButton.addClass('disabled');

                break;
            case HFProcess.ProcessGroupStates.STARTED:
                // if the process group is going, the stop button should be active
                toggleManageButton(stopButton, true);

                // enable the go button
                goButton.removeClass('disabled');

                break;
        }
    }

    $('#last-visited-link').click(function() {
        ipcRenderer.send('start-interface');
    });

    $('#go-server-button:not(.disabled)').click(function(){
        ipcRenderer.send('start-interface', { url: 'hifi://localhost' });
    })

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
    ipcRenderer.on('process-group-update', onProcessGroupUpdate);

    ipcRenderer.send('update-all-processes');
});
