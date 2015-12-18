'use strict'

var electron = require('electron');
var app = electron.app;  // Module to control application life.
var BrowserWindow = require('browser-window');  // Module to create native browser window.
var Menu = require('menu');
var Tray = require('tray');

var hfprocess = require('./modules/hf-process.js');
var Process = hfprocess.Process;
var ProcessGroup = hfprocess.ProcessGroup;

const ipcMain = electron.ipcMain;

// Report crashes to our server.
require('crash-reporter').start();

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the JavaScript object is garbage collected.
var mainWindow = null;
var tray = null;

var path = require('path');
var TRAY_ICON = path.join(__dirname, '../resources/console-tray.png');
var APP_ICON = path.join(__dirname, '../resources/console.png');

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
    tray = new Tray(TRAY_ICON);
    tray.setToolTip('High Fidelity Console');
    var contextMenu = Menu.buildFromTemplate([{
        label: 'Quit',
        accelerator: 'Command+Q',
        click: function() { app.quit(); }
    }]);
    tray.setContextMenu(contextMenu);

    // Require electron-compile to use LESS files in place of basic CSS
    require('electron-compile').init();

    // Create the browser window.
    mainWindow = new BrowserWindow({
        width: 800,
        height: 600,
        icon: APP_ICON,
        resizable: false
    });

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

    if (interfacePath && dsPath && acPath) {
        var pInterface = new Process('interface', interfacePath);

        var homeServer = new ProcessGroup('home', [
            new Process('Domain Server', dsPath),
            new Process('AC - Audio', acPath, ['-t0']),
            new Process('AC - Avatar', acPath, ['-t1']),
            new Process('AC - Asset', acPath, ['-t3']),
            new Process('AC - Messages', acPath, ['-t4']),
            new Process('AC - Entity', acPath, ['-t6'])
        ]);
        homeServer.start();

        // make sure we stop child processes on app quit
        app.on('quit', function(){
            pInterface.stop();
            homeServer.stop();
        })

        var processes = {
            interface: pInterface,
            home: homeServer
        };

        function sendProcessUpdate() {
            console.log("Sending process update to web view");
            mainWindow.webContents.send('process-update', processes);
        };

        pInterface.on('state-update', sendProcessUpdate);

        ipcMain.on('start-process', function(event, arg) {
            pInterface.start();
            sendProcessUpdate();
        });
        ipcMain.on('stop-process', function(event, arg) {
            pInterface.stop();
            sendProcessUpdate();
        });
        ipcMain.on('update', function(event, arg) {
            sendProcessUpdate();
        });

        sendProcessUpdate();
    }
});
