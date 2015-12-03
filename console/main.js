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
// be closed automatically when the JavaScript object is garggbage collected.
var mainWindow = null;
var appIcon = null;
var TRAY_ICON = 'resources/tray-icon.png';
var APP_ICON = 'resources/tray-icon.png';

// Quit when all windows are closed.
app.on('window-all-closed', function() {
<<<<<<< HEAD
    // On OS X it is common for applications and their menu bar
    // to stay active until the user quits explicitly with Cmd + Q
    if (process.platform != 'darwin') {
        app.quit();
    }
=======
      // On OS X it is common for applications and their menu bar
      // to stay active until the user quits explicitly with Cmd + Q
      if (process.platform != 'darwin') {
          app.quit();
      }
>>>>>>> add initial local process search
});

// Check command line arguments to see how to find binaries
var argv = require('yargs').argv;
var pathFinder = require('./path-finder.js');

var interfacePath = null;
var dsPath = null;
var acPath = null;

if (argv.localDebugBuilds || argv.localReleaseBuilds) {
    interfacePath = pathFinder.discoveredPath("Interface", argv.localReleaseBuilds);
    dsPath = pathFinder.discoveredPath("domain-server", argv.localReleaseBuilds);
    acPath = pathFinder.discoveredPath("assignment-client", argv.localReleaseBuilds);
}

// if at this point any of the paths are null, we're missing something we wanted to find

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

    var pInterface = new Process('interface', 'C:\\Interface\\interface.exe');

    var domainServerPath = 'C:\\Users\\Ryan\\AppData\\Local\\High Fidelity\\Stack Manager\\domain-server.exe';
    var acPath = 'C:\\Users\\Ryan\\AppData\\Local\\High Fidelity\\Stack Manager\\assignment-client.exe';

    var homeServer = new ProcessGroup('home', [
        new Process('Domain Server', domainServerPath),
        new Process('AC - Audio', acPath, ['-t0']),
        new Process('AC - Avatar', acPath, ['-t1']),
        new Process('AC - Asset', acPath, ['-t3']),
        new Process('AC - Messages', acPath, ['-t4']),
        new Process('AC - Entity', acPath, ['-t6'])
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
});
