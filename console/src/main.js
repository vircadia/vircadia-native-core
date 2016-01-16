'use strict';

const electron = require('electron');
const app = electron.app;  // Module to control application life.
const BrowserWindow = electron.BrowserWindow;

const dialog = electron.dialog;
const Menu = require('menu');
const Tray = require('tray');
const shell = require('shell');
const os = require('os');
const childProcess = require('child_process');
const path = require('path');
const fs = require('fs');
const Tail = require('always-tail');
const http = require('http');
const unzip = require('unzip');

const request = require('request');
const progress = require('request-progress');

const Config = require('./modules/config').Config;

const hfprocess = require('./modules/hf-process.js');
const Process = hfprocess.Process;
const ACMonitorProcess = hfprocess.ACMonitorProcess;
const ProcessStates = hfprocess.ProcessStates;
const ProcessGroup = hfprocess.ProcessGroup;
const ProcessGroupStates = hfprocess.ProcessGroupStates;

const osType = os.type();

const APP_ICON = path.join(__dirname, '../resources/console.png');

function getRootHifiDataDirectory() {
    if (osType == 'Windows_NT') {
        var homePath = process.env.HOMEPATH;
        return path.join(homePath, 'AppData/Roaming/High Fidelity');
    } else if (osType == 'Darwin') {
        return '~/Library/Application Support/High Fidelity';
    } else {
        return '/usr/local/share/High Fidelity';
    }
}

function getStackManagerDataDirectory() {
    // return path.join(getRootHifiDataDirectory(), '../../Local/High Fidelity');
}

function getAssignmentClientResourcesDirectory() {
    return path.join(getRootHifiDataDirectory(), '/assignment-client/resources');
}

function getApplicationDataDirectory() {
    return path.join(getRootHifiDataDirectory(), '/Console');
}

console.log("Root hifi directory is: ", getRootHifiDataDirectory());

const ipcMain = electron.ipcMain;


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
    console.warn("Another instance of the Server Console is already running - this instance will quit.");
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

var binaryType = argv.binaryType;

interfacePath = pathFinder.discoveredPath("Interface", binaryType);
dsPath = pathFinder.discoveredPath("domain-server", binaryType);
acPath = pathFinder.discoveredPath("assignment-client", binaryType);

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
    // Add quotes around path
    path = '"' + path + '"';
    if (osType == "Windows_NT") {
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

const GO_HOME_INDEX = 2;
const SERVER_LABEL_INDEX = 0;
const RESTART_INDEX = 4;
const STOP_INDEX = 5;
const SETTINGS_INDEX = 6;

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

function goHomeClicked() {
    if (interfacePath) {
        startInterface('hifi://localhost');
    } else {
        // show an error to say that we can't go home without an interface instance
        dialog.showErrorBox("Client Not Found", binaryMissingMessage("High Fidelity client", "Interface", false));
    }
}

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
                label: 'Server - Stopped',
                enabled: false
            },
            {
                type: 'separator'
            },
            {
                label: 'Go Home',
                click: goHomeClicked,
                enabled: false
            },
            {
                type: 'separator'
            },
            {
                label: 'Start Server',
                click: function() { homeServer.restart(); }
            },
            {
                label: 'Stop Server',
                visible: false,
                click: function() { homeServer.stop(); }
            },
            {
                label: 'Settings',
                click: function() { shell.openExternal('http://localhost:40100/settings'); },
                enabled: false
            },
            {
                label: 'View Logs',
                click: function() { logWindow.open(); }
            },
            {
                type: 'separator'
            },
            {
                label: 'Share',
                click: function() { shell.openExternal('http://localhost:40100/settings/?action=share') }
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
        restartItem.label = "Restart Server";
    } else if (serverState == ProcessGroupStates.STOPPED) {
        serverLabelItem.label = "Server - Stopped";
        restartItem.label = "Start Server";
    } else if (serverState == ProcessGroupStates.STOPPING) {
        serverLabelItem.label = "Server - Stopping";

        restartItem.label = "Restart Server";
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

    if (hasRun) {
        onComplete();
        return;
    }

    // Check for existing AC data
    const acResourceDirectory = getAssignmentClientResourcesDirectory();
    console.log("Checking for existence of " + acResourceDirectory);
    var userHasExistingServerData = true;
    try {
        fs.accessSync(acResourceDirectory);
    } catch (e) {
        console.log(e);
        userHasExistingServerData = false;
    }

    if (userHasExistingServerData) {
        console.log("User has existing data, suppressing downloader");
        onComplete();
        return;
    }

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
            // console.log(state, window, args);
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
                sendStateUpdate('downloading', { progress: state.percentage });
            }
        });
        var unzipper = unzip.Extract({
            path: acResourceDirectory,
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
            height: 737,
            center: true,
            frame: true,
            useContentSize: true,
            resizable: false
        });
        window.loadURL('file://' + __dirname + '/splash.html');
        // window.setMenu(null);
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
    tray.setToolTip('High Fidelity Server Console');

    tray.on('click', function() {
        tray.popUpContextMenu(tray.menu);
    });

    updateTrayMenu(ProcessGroupStates.STOPPED);

    maybeInstallDefaultContentSet(function() {
        maybeShowSplash();

        if (dsPath && acPath) {
            domainServer = new Process('domain-server', dsPath, [], logPath);
            acMonitor = new ACMonitorProcess('ac-monitor', acPath, ['-n6',
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
