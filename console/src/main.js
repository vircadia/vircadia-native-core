'use strict'

var electron = require('electron');
var app = electron.app;  // Module to control application life.
var BrowserWindow = require('browser-window');  // Module to create native browser window.
var Menu = require('menu');
var Tray = require('tray');
var shell = require('shell');
var os = require('os');
var childProcess = require('child_process');
var path = require('path');

var hfprocess = require('./modules/hf-process.js');
var Process = hfprocess.Process;
var ProcessGroup = hfprocess.ProcessGroup;

const ipcMain = electron.ipcMain;

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the JavaScript object is garbage collected.
var mainWindow = null;
var tray = null;

var path = require('path');
var TRAY_ICON = path.join(__dirname, '../resources/console-tray.png');
var APP_ICON = path.join(__dirname, '../resources/console.png');

// Don't quit when all windows are closed, make user explicitly quit from tray
app.on('window-all-closed', function() {});

function createNewWindow() {
    // Create the browser window.
    mainWindow = new BrowserWindow({
        title: "High Fidelity",
        width: 970,
        height: 775,
        icon: APP_ICON,
        resizable: false
    });

    // In debug mode, keep the menu bar, but auto-hide it so the UI still looks the same.
    if (debug) {
        mainWindow.setAutoHideMenuBar(true);
    } else {
        mainWindow.setMenu(null);
    }

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
}

// When a user clicks on dock icon, re-create the window if we don't have one
app.on('activate', function(){
    if (!mainWindow) {
        createNewWindow();
    }
})

var shouldQuit = app.makeSingleInstance(function(commandLine, workingDirectory) {
    // Someone tried to run a second instance, we should focus our window
    if (mainWindow) {
        if (mainWindow.isMinimized()) {
            mainWindow.restore();
        }
        mainWindow.focus();
    }
    return true;
});

if (shouldQuit) {
    app.quit();
    return;
}

// Check command line arguments to see how to find binaries
var argv = require('yargs').argv;
var pathFinder = require('./modules/path-finder.js');

var interfacePath = null;
var dsPath = null;
var acPath = null;

var debug = argv.debug;

if (argv.localDebugBuilds || argv.localReleaseBuilds) {
    interfacePath = pathFinder.discoveredPath("Interface", argv.localReleaseBuilds);
    dsPath = pathFinder.discoveredPath("domain-server", argv.localReleaseBuilds);
    acPath = pathFinder.discoveredPath("assignment-client", argv.localReleaseBuilds);
}

function openFileBrowser(path) {
    var type = os.type();
    console.log(type);
    if (type == "Windows_NT") {
        childProcess.exec('start ' + path);
    } else if (type == "Darwin") {
        childProcess.exec('open ' + path);
    } else if (type == "Linux") {
        childProcess.exec('xdg-open ' + path);
    }
}

// if at this point any of the paths are null, we're missing something we wanted to find
// TODO: show an error for the binaries that couldn't be found

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
app.on('ready', function() {
    // Create tray icon
    tray = new Tray(TRAY_ICON);
    tray.setToolTip('High Fidelity');
    var contextMenu = Menu.buildFromTemplate([{
        label: 'Quit',
        accelerator: 'Command+Q',
        click: function() { app.quit(); }
    }]);
    tray.setContextMenu(contextMenu);

    // Require electron-compile to use LESS files in place of basic CSS
    require('electron-compile').init();

    createNewWindow();

    var logPath = path.join(app.getAppPath(), 'logs');

    if (interfacePath && dsPath && acPath) {
        var homeServer = new ProcessGroup('home', [
            new Process('domain-server', dsPath),
            new Process('ac-monitor', acPath, ['-n6', '--log-directory', logPath])
        ]);

        // make sure we stop child processes on app quit
        app.on('quit', function(){
            homeServer.stop();
        });

        var processes = {
            home: homeServer
        };

        function sendProcessUpdate(process) {
            if (mainWindow) {
                console.log("Sending process update to web view for " + process.name);
                mainWindow.webContents.send('process-update', process);
            }
        };

        function sendProcessGroupUpdate(processGroup) {
            if (mainWindow) {
                mainWindow.webContents.send('process-group-update', processGroup);
            }
        }

        // handle process updates
        homeServer.on('process-update', sendProcessUpdate);
        homeServer.on('state-update', sendProcessGroupUpdate);

        // start the home server
        homeServer.start();

        ipcMain.on('start-interface', function(event, arg) {
            // check if we have a url parameter to include
            var argArray = [];
            if (arg && arg.url) {
                argArray = ["--url", arg.url];
            }

            // create a new Interface instance - Interface makes sure only one is running at a time
            var pInterface = new Process('interface', interfacePath, argArray);
            pInterface.start();
        });

        ipcMain.on('restart-server', function(event, arg) {
            homeServer.restart();
        });

        ipcMain.on('stop-server', function(event, arg) {
            homeServer.stop();
        });

        ipcMain.on('open-logs', function(event, arg) {
            openFileBrowser(logPath);
        });

        ipcMain.on('update-all-processes', function(event, arg) {
            // enumerate our processes and call sendProcessUpdate to update
            // the window with their status
            for (let process of homeServer.processes) {
                sendProcessUpdate(process);
            }

            sendProcessGroupUpdate(homeServer);
        });
    }
});
