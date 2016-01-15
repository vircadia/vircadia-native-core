'use strict';

var electron = require('electron');
var app = electron.app;  // Module to control application life.
var BrowserWindow = electron.BrowserWindow;

var dialog = electron.dialog;
var Menu = require('menu');
var Tray = require('tray');
var shell = require('shell');
var os = require('os');
var childProcess = require('child_process');
var path = require('path');
var fs = require('fs');
var Tail = require('always-tail');
var http = require('http');
var path = require('path');
var unzip = require('unzip');

var request = require('request');
var progress = require('request-progress');

var Config = require('./modules/config').Config;

var hfprocess = require('./modules/hf-process.js');
var Process = hfprocess.Process;
var ACMonitorProcess = hfprocess.ACMonitorProcess;
var ProcessStates = hfprocess.ProcessStates;
var ProcessGroup = hfprocess.ProcessGroup;
var ProcessGroupStates = hfprocess.ProcessGroupStates;


function getRootHifiDataDirectory() {
    var rootDirectory = app.getPath('appData');
    return path.join(rootDirectory, '/High Fidelity');
}

function getStackManagerDataDirectory() {
    return path.join(getRootHifiDataDirectory(), "../../Local/High Fidelity");
}

function getAssignmentClientResourcesDirectory() {
    return path.join(getRootHifiDataDirectory(), '/assignment-client/resources');
}

function getApplicationDataDirectory() {
    return path.join(getRootHifiDataDirectory(), '/Console');
}


console.log("Root directory is: ", getRootHifiDataDirectory());

const ipcMain = electron.ipcMain;

const osType = os.type();

var isShuttingDown = false;
function shutdown() {
    if (!isShuttingDown) {
        var idx = dialog.showMessageBox({
            type: 'question',
            buttons: ['Yes', 'No'],
            title: 'High Fidelity',
            message: 'Are you sure you want to quit?'
        });
        if (idx == 0) {
            isShuttingDown = true;
            if (logWindow) {
                logWindow.close();
            }
            if (homeServer) {
                homeServer.stop();
            }

            var timeoutID = setTimeout(app.quit, 5000);
            homeServer.on('state-update', function(processGroup) {
                if (processGroup.state == ProcessGroupStates.STOPPED) {
                    clearTimeout(timeoutID);
                    app.quit();
                }
            });

            updateTrayMenu(null);
        }
    }
}


var logPath = path.join(getApplicationDataDirectory(), '/logs');
console.log("Log directory:", logPath);

const configPath = path.join(getApplicationDataDirectory(), 'config.json');
var userConfig = new Config();
userConfig.load(configPath);

const TRAY_FILENAME = (osType == "Darwin" ? "console-tray-Template.png" : "console-tray.png");
const TRAY_ICON = path.join(__dirname, '../resources/' + TRAY_FILENAME);
const APP_ICON = path.join(__dirname, '../resources/console.png');

// print out uncaught exceptions in the console
process.on('uncaughtException', function(err) {
    console.error(err);
    console.error(err.stack);
});

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

// if at this point any of the paths are null, we're missing something we wanted to find
// TODO: show an error for the binaries that couldn't be found

function openFileBrowser(path) {
    // Add quotes around path
    path = '"' + path + '"';
    if (osType == "Windows_NT") {
        console.log('start "" ' + path);
        childProcess.exec('start "" ' + path);
    } else if (osType == "Darwin") {
        childProcess.exec('open ' + path);
    } else if (osType == "Linux") {
        childProcess.exec('xdg-open ' + path);
    }
}

app.on('window-all-closed', function() {
});

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
global.homeServer = null;
global.domainServer = null;
global.acMonitor = null;
global.userConfig = userConfig;

const GO_HOME_INDEX = 0;
const SERVER_LABEL_INDEX = 2;
const RESTART_INDEX = 3;
const STOP_INDEX = 4;
const SETTINGS_INDEX = 5;

var LogWindow = function(ac, ds) {
    this.ac = ac;
    this.ds = ds;
    this.window = null;
    this.acMonitor = null;
    this.dsMonitor = null;
}
LogWindow.prototype = {
    open: function() {
        if (this.window) {
            this.window.show();
            this.window.restore();
            return;
        }
        // Create the browser window.
        this.window = new BrowserWindow({ width: 700, height: 500, icon: APP_ICON });
        this.window.loadURL('file://' + __dirname + '/log.html');

        if (!debug) {
            logWindow.setMenu(null);
        }

        this.window.on('closed', function() {
            this.window = null;
        }.bind(this));
    },
    close: function() {
        if (this.window) {
            this.window.close();
        }
    }
};

var logWindow = null;

function buildMenuArray(serverState) {
    var menuArray = null;
    if (isShuttingDown) {
        menuArray = [
            {
                label: "Shutting down...",
                enabled: false
            }
        ];
    } else {
        menuArray = [
            {
                label: 'Go Home',
                click: function() { startInterface('hifi://localhost'); },
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
                click: function() { logWindow.open(); }
            },
            {
                label: "Open Log Directory",
                click: function() { openFileBrowser(logPath); }
            },
            {
                type: 'separator'
            },
            {
                label: 'Quit',
                accelerator: 'Command+Q',
                click: function() { shutdown(); }
            }
        ];

        updateMenuArray(menuArray, serverState);
    }


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
        restartItem.enabled = false;
    }
}

function updateTrayMenu(serverState) {
    if (tray) {
        var menuArray = buildMenuArray(serverState);
        tray.setContextMenu(Menu.buildFromTemplate(menuArray));
        if (isShuttingDown) {
            tray.setToolTip('High Fidelity - Shutting Down');
        }
    }
}

const httpStatusPort = 60332;

function maybeInstallDefaultContentSet(onComplete) {
    var hasRun = userConfig.get('hasRun', false);

    if (false && hasRun) {
        return;
    }

    // Check for existing Stack Manager data
    const stackManagerDataPath = getStackManagerDataDirectory();
    console.log("Checking for existence of " + stackManagerDataPath);
    var userHasExistingServerData = true;
    try {
        fs.accessSync(stackManagerDataPath);
    } catch (e) {
        console.log(e);
        userHasExistingServerData = false;
    }

    console.log("Existing data?", userHasExistingServerData);

    // Show popup
    var window = new BrowserWindow({
        icon: APP_ICON,
        width: 640,
        height: 480,
        center: true,
        frame: true,
        useContentSize: true,
        resizable: false
    });
    window.loadURL('file://' + __dirname + '/downloader.html');
    window.setMenu(null);
    window.show();

    window.on('closed', onComplete);

    electron.ipcMain.on('ready', function() {
        console.log("got ready");
        function sendStateUpdate(state, args) {
            console.log(state, window, args);
            window.webContents.send('update', { state: state, args: args });
        }

        var aborted = false;

        // Start downloading content set
        var req = progress(request.get({
            url: "https://s3.amazonaws.com/hifi-public/homeset/updated.zip"
        }, function(error, responseMessage, responseData) {
            if (aborted) {
                return;
            } else if (error || responseMessage.statusCode != 200) {
                var message = '';
                if (error) {
                    message = "Error contacting resource server.";
                } else {
                    message = "Error downloading resources from server.";
                }
                sendStateUpdate('error', {
                    message: message
                });
            } else {
                sendStateUpdate('installing');
            }
        }), { throttle: 250 }).on('progress', function(state) {
            if (!aborted) {
                // Update progress popup
                console.log("progress", state);
                sendStateUpdate('downloading', { progress: state.percentage });
            }
        });
        var unzipper = unzip.Extract({
            path: getAssignmentClientResourcesDirectory(),
            verbose: true
        });
        unzipper.on('close', function() {
            console.log("Done", arguments);
            sendStateUpdate('complete');
        });
        unzipper.on('error', function (err) {
            console.log("aborting");
            aborted = true;
            req.abort();
            console.log("ERROR");
            sendStateUpdate('error', {
                message: "Error installing resources."
            });
        });
        req.pipe(unzipper);


        userConfig.set('hasRun', true);
    });
}

function maybeShowSplash() {
    var suppressSplash = userConfig.get('doNotShowSplash', false);

    if (!suppressSplash) {
        var window = new BrowserWindow({
            icon: APP_ICON,
            width: 1600,
            height: 587,
            center: true,
            frame: true,
            useContentSize: true,
            resizable: false
        });
        window.loadURL('file://' + __dirname + '/splash.html');
        window.setMenu(null);
        window.show();

        window.webContents.on('new-window', function(e, url) {
            e.preventDefault();
            require('shell').openExternal(url);
        });
    }
}

function detectExistingStackManagerResources() {
    return false;
}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
app.on('ready', function() {

    if (app.dock) {
        // hide the dock icon on OS X
        app.dock.hide();
    }

    // Create tray icon
    tray = new Tray(TRAY_ICON);
    tray.setToolTip('High Fidelity');

    updateTrayMenu(ProcessGroupStates.STOPPED);

    maybeInstallDefaultContentSet(function() {
        maybeShowSplash();

        if (interfacePath && dsPath && acPath) {
            domainServer = new Process('domain-server', dsPath, [], logPath);
            acMonitor = new ACMonitorProcess('ac-monitor', acPath, ['-n4',
                                                                        '--log-directory', logPath,
                                                                        '--http-status-port', httpStatusPort], httpStatusPort, logPath);
            homeServer = new ProcessGroup('home', [domainServer, acMonitor]);
            logWindow = new LogWindow(acMonitor, domainServer);

            // make sure we stop child processes on app quit
            app.on('quit', function(){
                console.log('App quitting');
                userConfig.save(configPath);
                logWindow.close();
                homeServer.stop();
            });

            var processes = {
                home: homeServer
            };

            // handle process updates
            homeServer.on('state-update', function(processGroup) { updateTrayMenu(processGroup.state); });

            // start the home server
            homeServer.start();
        }
    });
});
