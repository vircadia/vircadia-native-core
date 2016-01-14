'use strict'

var electron = require('electron');
var app = electron.app;  // Module to control application life.
var BrowserWindow = electron.BrowserWindow;

var Menu = require('menu');
var Tray = require('tray');
var shell = require('shell');
var os = require('os');
var childProcess = require('child_process');
var path = require('path');
const dialog = require('electron').dialog;

var hfprocess = require('./modules/hf-process.js');
var Process = hfprocess.Process;
var ProcessGroup = hfprocess.ProcessGroup;
var ProcessGroupStates = hfprocess.ProcessGroupStates;

const ipcMain = electron.ipcMain;

const osType = os.type();

var path = require('path');

const TRAY_FILENAME = (osType == "Darwin" ? "console-tray-Template.png" : "console-tray.png");
const TRAY_ICON = path.join(__dirname, '../resources/' + TRAY_FILENAME);

// print out uncaught exceptions in the console
process.on('uncaughtException', console.log.bind(console));

var shouldQuit = app.makeSingleInstance(function(commandLine, workingDirectory) {
    // Someone tried to run a second instance, focus the window (if there is one)
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

function binaryMissingMessage(displayName, executableName, required) {
    var message = "The " + displayName + " executable was not found.\n";

    if (required) {
        message += "It is required for the Server Console to run.\n\n";
    } else {
        message += "\n";
    }

    if (debug) {
        message += "Please ensure there is a compiled " + displayName + " in a folder named build in this checkout.\n\n";
        message += "It was expected to be found at one of the following paths:\n";

        var paths = pathFinder.searchPaths(executableName, argv.localReleaseBuilds);
        message += paths.join("\n");
    } else {
        message += "It is expected to be found beside this executable.\n"
        message += "You may need to re-install the Server Console.";
    }

    return message;
}

// if at this point any of the paths are null, we're missing something we wanted to find

if (!dsPath) {
    dialog.showErrorBox("Domain Server Not Found", binaryMissingMessage("domain-server", "domain-server", true));
    app.quit();
}

if (!acPath) {
    dialog.showErrorBox("Assignment Client Not Found", binaryMissingMessage("assignment-client", "assignment-client", true))
    app.quit();
}

function openFileBrowser(path) {
    if (osType == "Windows_NT") {
        childProcess.exec('start ' + path);
    } else if (osType == "Darwin") {
        childProcess.exec('open ' + path);
    } else if (osType == "Linux") {
        childProcess.exec('xdg-open ' + path);
    }
}

function startInterface(url) {
    var argArray = [];

    // check if we have a url parameter to include
    if (url) {
        argArray = ["--url", url];
    }

    // create a new Interface instance - Interface makes sure only one is running at a time
    var pInterface = new Process('interface', interfacePath, argArray);
    pInterface.start();
}

var tray = null;
var logPath = null;
var homeServer = null;

const GO_HOME_INDEX = 0;
const SERVER_LABEL_INDEX = 2;
const RESTART_INDEX = 3;
const STOP_INDEX = 4;
const SETTINGS_INDEX = 5;

function goHomeClicked() {
    if (interfacePath) {
        startInterface('hifi://localhost');
    } else {
        // show an error to say that we can't go home without an interface instance
        dialog.showErrorBox("Client Not Found", binaryMissingMessage("High Fidelity Client", "Interface", false));
    }
}

function buildMenuArray(serverState) {
    var menuArray = [
        {
            label: 'Go Home',
            click: goHomeClicked,
            enabled: false
        },
        {
            type: 'separator'
        },
        {
            label: "Server - Stopped",
            enabled: false
        },
        {
            label: "Start",
            click: function() { homeServer.restart(); }
        },
        {
            label: "Stop",
            visible: false,
            click: function() { homeServer.stop(); }
        },
        {
            label: "Settings",
            click: function() { shell.openExternal('http://localhost:40100/settings'); },
            enabled: false
        },
        {
            label: "View Logs",
            click: function() { openFileBrowser(logPath); }
        },
        {
            type: 'separator'
        },
        {
            label: 'Quit',
            accelerator: 'Command+Q',
            click: function() { app.quit(); }
        }
    ];

    updateMenuArray(menuArray, serverState);

    return menuArray;
}

function updateMenuArray(menuArray, serverState) {
    // update the tray menu state
    var running = serverState == ProcessGroupStates.STARTED;

    var serverLabelItem = menuArray[SERVER_LABEL_INDEX];
    var restartItem = menuArray[RESTART_INDEX];

    // Go Home is only enabled if running
    menuArray[GO_HOME_INDEX].enabled = running;

    // Stop is only visible if running
    menuArray[STOP_INDEX].visible = running;

    // Settings is only visible if running
    menuArray[SETTINGS_INDEX].enabled = running;

    if (serverState == ProcessGroupStates.STARTED) {
        serverLabelItem.label = "Server - Started";
        restartItem.label = "Restart";
    } else if (serverState == ProcessGroupStates.STOPPED) {
        serverLabelItem.label = "Server - Stopped";
        restartItem.label = "Start";
    } else if (serverState == ProcessGroupStates.STOPPING) {
        serverLabelItem.label = "Server - Stopping";
        restartItem.label = "Restart";
    }
}

function updateTrayMenu(serverState) {
    if (tray) {
        var menuArray = buildMenuArray(serverState);
        tray.setContextMenu(Menu.buildFromTemplate(menuArray));
    }
}

var hiddenWindow = null;

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
app.on('ready', function() {

    if (app.dock) {
        // hide the dock icon on OS X
        app.dock.hide()
    }

    logPath = path.join(app.getAppPath(), 'logs');

    // Create tray icon
    tray = new Tray(TRAY_ICON);
    tray.setToolTip('High Fidelity Server Console');

    tray.on('click', function() {
        tray.popUpContextMenu(tray.menu);
    });

    updateTrayMenu(ProcessGroupStates.STOPPED);

    if (dsPath && acPath) {
        homeServer = new ProcessGroup('home', [
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

        // handle process updates
        // homeServer.on('process-update', sendProcessUpdate);
        homeServer.on('state-update', function(processGroup) { updateTrayMenu(processGroup.state); });

        // start the home server
        homeServer.start();
    }
});
