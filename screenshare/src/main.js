'use strict';

const {app, BrowserWindow, ipcMain} = require('electron');
const gotTheLock = app.requestSingleInstanceLock()
const argv = require('yargs').argv;
// ./screenshare.exe --userName=miladN ...
const connectionInfo = {
    token: argv.token || "token",
    projectAPIKey: argv.projectAPIKey || "projectAPIKey",
    sessionID: argv.sessionID || "sessionID"
}

const osType = require('os').type();
let width;
let height;
if (osType == "Darwin" || osType == "Linux") {
    width = 960;
    height = 660;
} else if (osType == "Windows_NT") {
    width = 973;
    height = 740;
}

if (!gotTheLock) {
  console.log("Another instance of the screenshare is already running - this instance will quit.");
  app.exit(0);
  return;
}

let window;
const zoomFactor = 1.0;
function createWindow(){
    console.log("Creating window")
    window = new BrowserWindow({
        backgroundColor: "#000000",
        width: width,
        height: height,
        center: true,
        frame: true,
        useContentSize: true,
        zoomFactor: zoomFactor,
        resizable: false,
        webPreferences: {
            nodeIntegration: true
        }
    });
    window.loadURL('file://' + __dirname + '/index.html');
    window.setMenu(null);
    
    window.webContents.on("did-finish-load", function(){
        console.log("connectionInfo:", connectionInfo)
        console.log("in did finish loading");
        window.webContents.send('connectionInfo', JSON.stringify(connectionInfo));
    });

}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
console.log("setting up on ready");
app.on('ready', function() {
    console.log("app ready");
    createWindow();
    console.log("sending info");
    window.webContents.send('connectionInfo', JSON.stringify(connectionInfo))
});
