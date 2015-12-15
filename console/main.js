'use strict'

var electron = require('electron');
var app = electron.app;  // Module to control application life.
var BrowserWindow = require('browser-window');  // Module to create native browser window.
var Menu = require('menu');
var Tray = require('tray');
var shell = require('shell');

var hfprocess = require('./modules/hf-process.js');
var Process = hfprocess.Process;
var ProcessGroup = hfprocess.ProcessGroup;

const ipcMain = electron.ipcMain;

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the JavaScript object is garbage collected.
var mainWindow = null;
var appIcon = null;
var TRAY_ICON = 'resources/tray-icon.png';
var APP_ICON = 'resources/tray-icon.png';

// Quit when all windows are closed.
app.on('window-all-closed', function() {
    // On OS X it is common for applications and their menu bar
    // to stay active until the user quits explicitly with Cmd + Q
    if (process.platform != 'darwin') {
        app.quit();
    }
});

// Check command line arguments to see how to find binaries
var argv = require('yargs').argv;
var pathFinder = require('./modules/path-finder.js');

var interfacePath = null;
var dsPath = null;
var acPath = null;

if (argv.localDebugBuilds || argv.localReleaseBuilds) {
    interfacePath = pathFinder.discoveredPath("Interface", argv.localReleaseBuilds);
    dsPath = pathFinder.discoveredPath("domain-server", argv.localReleaseBuilds);
    acPath = pathFinder.discoveredPath("assignment-client", argv.localReleaseBuilds);
}

// if at this point any of the paths are null, we're missing something we wanted to find
// TODO: show an error for the binaries that couldn't be found

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
app.on('ready', function() {
    // Create tray icon
    appIcon = new Tray(TRAY_ICON);
    appIcon.setToolTip('High Fidelity Console');
    var contextMenu = Menu.buildFromTemplate([{
        label: 'Quit',
        accelerator: 'Command+Q',
        click: function() { app.quit(); }
    }]);
    appIcon.setContextMenu(contextMenu);

    // Create the browser window.
    mainWindow = new BrowserWindow({width: 800, height: 600, icon: APP_ICON});

    // and load the index.html of the app.
    mainWindow.loadURL('file://' + __dirname + '/index.html');

    // Open the DevTools.
    mainWindow.webContents.openDevTools();

    // Emitted when the window is closed.
    mainWindow.on('closed', function() {
        // Dereference the window object, usually you would store windows
        // in an array if your app supports multi windows, this is the time
        // when you should delete the corresponding element.
        mainWindow = null;
    });

    // When a link is clicked that has `_target="_blank"`, open it in the user's native browser
    mainWindow.webContents.on('new-window', function(e, url) {
        e.preventDefault();
        shell.openExternal(url);
    });

    if (interfacePath && dsPath && acPath) {
        var pInterface = new Process('interface', interfacePath);

        var homeServer = new ProcessGroup('home', [
            new Process('domain_server', dsPath),
            new Process('ac_audio', acPath, ['-t0']),
            new Process('ac_avatar', acPath, ['-t1']),
            new Process('ac_agent', acPath, ['-t2']),
            new Process('ac_asset', acPath, ['-t3']),
            new Process('ac_messages', acPath, ['-t4']),
            new Process('ac_entity', acPath, ['-t6'])
        ]);
        homeServer.start();

        var processes = {
            interface: pInterface,
            home: homeServer
        };

        function sendProcessUpdate() {
            console.log("Sending process update to web view");
            mainWindow.webContents.send('process-update', processes);
        };

        pInterface.on('state-update', sendProcessUpdate);
        homeServer.on('state-update', sendProcessUpdate);

        ipcMain.on('start-process', function(event, arg) {
            pInterface.start();
            sendProcessUpdate();
        });
        ipcMain.on('stop-process', function(event, arg) {
            pInterface.stop();
            sendProcessUpdate();
        });
        ipcMain.on('start-server', function(event, arg) {
            homeServer.start();
            sendProcessUpdate();
        });
        ipcMain.on('stop-server', function(event, arg) {
            homeServer.stop();
            sendProcessUpdate();
        });
        ipcMain.on('update', sendProcessUpdate);

        sendProcessUpdate();
    }
});
