function ready() {
    console.log("Ready");

    const electron = require('electron');
    window.$ = require('./vendor/jquery/jquery-2.1.4.min.js');

    electron.ipcRenderer.on('update', function(event, message) {
        $('#directory').html(message);

        electron.ipcRenderer.send('setSize',  {
            width: $(window).width(),
            height:  $('#content').height() + 50
        });
    });

    electron.ipcRenderer.send('ready');
}
