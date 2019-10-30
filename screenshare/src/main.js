'use strict';

var userName, displayName, token, apiKey, sessionID;

const {app, BrowserWindow, ipcMain} = require('electron');
const gotTheLock = app.requestSingleInstanceLock()
const argv = require('yargs').argv;
// ./screenshare.exe --userName=miladN ...

const connectionInfo = {
    userName: argv.userName || "testName",
    displayName: argv.displayName || "displayName",
    token: argv.token || "token",
    apiKey: argv.apiKey || "apiKey",
    sessionID: argv.sessionID || "sessionID"
}

if (!gotTheLock) {
//   log.warn("Another instance of the screenshare is already running - this instance will quit.");
  app.exit(0);
  return;
}

var window;
function createWindow(){
    const zoomFactor = 1.0;
    window = new BrowserWindow({
        backgroundColor: "#000000",
        width: 1280 * zoomFactor,
        height: 720 * zoomFactor,
        center: true,
        frame: true,
        useContentSize: true,
        zoomFactor: zoomFactor,
        resizable: false,
        alwaysOnTop: true, // TRY
        webPreferences: {
            nodeIntegration: true
        },
        devTools: true
    });
    window.loadURL('file://' + __dirname + '/index.html');
    window.setMenu(null);

    window.once('ready-to-show', () => {
        window.show();       
        window.webContents.openDevTools()
    })
}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
app.on('ready', function() {
    createWindow();
    console.log("sending info");
    window.webContents.send('connectionInfo', JSON.stringify(connectionInfo))
});