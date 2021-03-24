'use strict';

const electron = require('electron');
const app = electron.app;  // Module to control application life.
const BrowserWindow = electron.BrowserWindow;
const nativeImage = electron.nativeImage;

const notifier = require('node-notifier');
const util = require('util');
const dialog = electron.dialog;
const Menu = electron.Menu;
const Tray = electron.Tray;
const shell = electron.shell;
const os = require('os');
const childProcess = require('child_process');
const path = require('path');
const fs = require('fs-extra');
const Tail = require('always-tail');
const http = require('http');
const zlib = require('zlib');
const tar = require('tar-fs');

const request = require('request');
const progress = require('request-progress');
const osHomeDir = require('os-homedir');

const updater = require('./modules/hf-updater.js');

const Config = require('./modules/config').Config;

const hfprocess = require('./modules/hf-process.js');

global.log = require('electron-log');

const Process = hfprocess.Process;
const ACMonitorProcess = hfprocess.ACMonitorProcess;
const ProcessStates = hfprocess.ProcessStates;
const ProcessGroup = hfprocess.ProcessGroup;
const ProcessGroupStates = hfprocess.ProcessGroupStates;

const hfApp = require('./modules/hf-app.js');
const GetBuildInfo = hfApp.getBuildInfo;
const StartInterface = hfApp.startInterface;
const getRootHifiDataDirectory = hfApp.getRootHifiDataDirectory;
const getDomainServerClientResourcesDirectory = hfApp.getDomainServerClientResourcesDirectory;
const getAssignmentClientResourcesDirectory = hfApp.getAssignmentClientResourcesDirectory;
const getApplicationDataDirectory = hfApp.getApplicationDataDirectory;


const osType = os.type();

const appIcon = path.join(__dirname, '../resources/console.png');

const menuNotificationIcon = path.join(__dirname, '../resources/tray-menu-notification.png');

const DELETE_LOG_FILES_OLDER_THAN_X_SECONDS = 60 * 60 * 24 * 7; // 7 Days
const LOG_FILE_REGEX = /(domain-server|ac-monitor|ac)-.*-std(out|err).txt/;

const HOME_CONTENT_URL = "http://cdn-1.vircadia.com/us-e-1/DomainContent/Sandbox/Rearranged_Basic_Sandbox.tar.gz";

const buildInfo = GetBuildInfo();

const NotificationState = {
    UNNOTIFIED: 'unnotified',
    NOTIFIED: 'notified'
};

// Update lock filepath
const UPDATER_LOCK_FILENAME = ".updating";
const UPDATER_LOCK_FULL_PATH = getRootHifiDataDirectory() + "/" + UPDATER_LOCK_FILENAME;

// Configure log
const oldLogFile = path.join(getApplicationDataDirectory(), '/log.txt');
const logFile = path.join(getApplicationDataDirectory(true), '/log.txt');
if (oldLogFile != logFile && fs.existsSync(oldLogFile)) {
    if (!fs.existsSync(oldLogFile)) {
        fs.moveSync(oldLogFile, logFile);
    } else {
        fs.remove(oldLogFile);
    }
}
fs.ensureFileSync(logFile); // Ensure file exists
log.transports.file.maxSize = 5 * 1024 * 1024;
log.transports.file.file = logFile;

log.debug("build info", buildInfo);
log.debug("Root hifi directory is: ", getRootHifiDataDirectory());
log.debug("App Data directory:", getApplicationDataDirectory());
fs.ensureDirSync(getApplicationDataDirectory());

var oldLogPath = path.join(getApplicationDataDirectory(), '/logs');
var logPath = path.join(getApplicationDataDirectory(true), '/logs');
if (oldLogPath != logPath && fs.existsSync(oldLogPath)) {
    if (!fs.existsSync(oldLogPath)) {
        fs.moveSync(oldLogPath, logPath);
    } else {
        fs.remove(oldLogPath);
    }
}
fs.ensureDirSync(logPath);
log.debug("Log directory:", logPath);

const configPath = path.join(getApplicationDataDirectory(), 'config.json');
var userConfig = new Config();
userConfig.load(configPath);

const ipcMain = electron.ipcMain;


function isInterfaceInstalled () {
    if (osType == "Darwin") {
        // In OSX Sierra, the app translocation process moves
        // the executable to a random location before starting it
        // which makes finding the interface near impossible using
        // relative paths.  For now, as there are no server-only
        // installs, we just assume the interface is installed here
        return true;
    } else {
        return interfacePath;
    }
}

function isServerInstalled () {
    return dsPath && acPath;
}

var isShuttingDown = false;
function shutdown() {
    log.debug("Normal shutdown (isShuttingDown: " + isShuttingDown +  ")");
    if (!isShuttingDown) {
        // if the home server is running, show a prompt before quit to ask if the user is sure
        if (isServerInstalled() && homeServer.state == ProcessGroupStates.STARTED) {
            log.debug("Showing shutdown dialog.");
            dialog.showMessageBox({
                type: 'question',
                buttons: ['Yes', 'No'],
                title: 'Stopping Vircadia Sandbox',
                message: 'Quitting will stop your Sandbox and your Home domain will no longer be running.\nDo you wish to continue?'
            }, shutdownCallback);
        } else {
            shutdownCallback(0);
        }

    }
}

function forcedShutdown() {
    log.debug("Forced shutdown (isShuttingDown: " + isShuttingDown +  ")");
    if (!isShuttingDown) {
        shutdownCallback(0);
    }
}

function shutdownCallback(idx) {
    log.debug("Entering shutdown callback.");
    if (idx == 0 && !isShuttingDown) {
        isShuttingDown = true;

        log.debug("Stop tray polling.");
        trayNotifications.stopPolling();

        log.debug("Saving user config");
        userConfig.save(configPath);

        if (logWindow) {
            log.debug("Closing log window");
            logWindow.close();
        }

        if (isServerInstalled()) {
            if (homeServer) {
                log.debug("Stoping home server");
                homeServer.stop();

                updateTrayMenu(homeServer.state);

                if (homeServer.state == ProcessGroupStates.STOPPED) {
                    // if the home server is already down, take down the server console now
                    log.debug("Quitting.");
                    app.exit(0);
                } else {
                    // if the home server is still running, wait until we get a state change or timeout
                    // before quitting the app
                    log.debug("Server still shutting down. Waiting");
                    var timeoutID = setTimeout(function() {
                        app.exit(0);
                    }, 5000);
                    homeServer.on('state-update', function(processGroup) {
                        if (processGroup.state == ProcessGroupStates.STOPPED) {
                            clearTimeout(timeoutID);
                            log.debug("Quitting.");
                            app.exit(0);
                        }
                    });
                }
            }
        }
        else {
            app.exit(0);
        }
    }
}

function deleteOldFiles(directoryPath, maxAgeInSeconds, filenameRegex) {
    log.debug("Deleting old log files in " + directoryPath);

    var filenames = [];
    try {
        filenames = fs.readdirSync(directoryPath);
    } catch (e) {
        log.warn("Error reading contents of log file directory", e);
        return;
    }

    for (const filename of filenames) {
        log.debug("Checking", filename);
        const absolutePath = path.join(directoryPath, filename);
        var stat = null;
        try {
            stat = fs.statSync(absolutePath);
        } catch (e) {
            log.debug("Error stat'ing file", absolutePath, e);
            continue;
        }
        const curTime = Date.now();
        if (stat.isFile() && filename.search(filenameRegex) >= 0) {
            const ageInSeconds = (curTime - stat.mtime.getTime()) / 1000.0;
            if (ageInSeconds >= maxAgeInSeconds) {
                log.debug("\tDeleting:", filename, ageInSeconds);
                try {
                    fs.unlinkSync(absolutePath);
                } catch (e) {
                    if (e.code != 'EBUSY') {
                        log.warn("\tError deleting:", e);
                    }
                }
            }
        }
    }
}

app.setAppUserModelId(buildInfo.appUserModelId);

// print out uncaught exceptions in the console
process.on('uncaughtException', function(err) {
    log.error(err);
    log.error(err.stack);
});

const gotTheLock = app.requestSingleInstanceLock()

if (!gotTheLock) {
  log.warn("Another instance of the Sandbox is already running - this instance will quit.");
  app.exit(0);
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

interfacePath = pathFinder.discoveredPath("interface", binaryType, buildInfo.releaseType);
dsPath = pathFinder.discoveredPath("domain-server", binaryType, buildInfo.releaseType);
acPath = pathFinder.discoveredPath("assignment-client", binaryType, buildInfo.releaseType);

console.log("Domain Server Path: " + dsPath);
console.log("Assignment Client Path: " + acPath);
console.log("Interface Path: " + interfacePath);

function binaryMissingMessage(displayName, executableName, required) {
    var message = "The " + displayName + " executable was not found.\n";

    if (required) {
        message += "It is required for the Vircadia Sandbox to run.\n\n";
    } else {
        message += "\n";
    }

    if (debug) {
        message += "Please ensure there is a compiled " + displayName + " in a folder named build in this checkout.\n\n";
        message += "It was expected to be found at one of the following paths:\n";

        var paths = pathFinder.searchPaths(executableName, argv.localReleaseBuilds);
        message += paths.join("\n");
    } else {
        message += "It is expected to be found beside this executable.\n";
        message += "You may need to re-install the Vircadia Sandbox.";
    }

    return message;
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

function openLogDirectory() {
    openFileBrowser(logPath);
}

// NOTE: this looks like it does nothing, but it's very important.
// Without it the default behaviour is to quit the app once all windows closed
// which is absolutely not what we want for a taskbar application.
app.on('window-all-closed', function() {
});

var tray = null;
global.homeServer = null;
global.domainServer = null;
global.acMonitor = null;
global.userConfig = userConfig;
global.openLogDirectory = openLogDirectory;

const hfNotifications = require('./modules/hf-notifications.js');
const HifiNotifications = hfNotifications.HifiNotifications;
const HifiNotificationType = hfNotifications.NotificationType;

var pendingNotifications = {}
var notificationState = NotificationState.UNNOTIFIED;

function setNotificationState (notificationType, pending = undefined) {
    if (pending !== undefined) {
        if ((notificationType === HifiNotificationType.TRANSACTIONS ||
            notificationType === HifiNotificationType.ITEMS)) {
            // special case, because we want to clear the indicator light
            // on INVENTORY when either Transactions or Items are
            // clicked on in the notification popup, we detect that case
            // here and force both to be unnotified.
            pendingNotifications[HifiNotificationType.TRANSACTIONS] = pending;
            pendingNotifications[HifiNotificationType.ITEMS] = pending;
        } else {
            pendingNotifications[notificationType] = pending;
        }
        notificationState = NotificationState.UNNOTIFIED;
        for (var key in pendingNotifications) {
            if (pendingNotifications[key]) {
                notificationState = NotificationState.NOTIFIED;
                break;
            }
        }
    }
    updateTrayMenu(homeServer ? homeServer.state : ProcessGroupStates.STOPPED);
}

var trayNotifications = new HifiNotifications(userConfig, setNotificationState);

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
        this.window = new BrowserWindow({ width: 700, height: 500, icon: appIcon });
        this.window.loadURL('file://' + __dirname + '/log.html');

        if (!debug) {
            this.window.setMenu(null);
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

function visitSandboxClicked() {
    if (isInterfaceInstalled()) {
        StartInterface('hifi://localhost');
    } else {
        // show an error to say that we can't go home without an interface instance
        dialog.showErrorBox("Client Not Found", binaryMissingMessage("Vircadia client", "Interface", false));
    }
}

var logWindow = null;

var labels = {
    serverState: {
        label: 'Server - Stopped',
        enabled: false
    },
    version: {
        label: 'Version - ' + buildInfo.buildIdentifier,
        enabled: false
    },
    showNotifications: {
        label: 'Show Notifications',
        type: 'checkbox',
        checked: true,
        click: function () {
            trayNotifications.enable(!trayNotifications.enabled(), setNotificationState);
            userConfig.save(configPath);
            updateTrayMenu(homeServer ? homeServer.state : ProcessGroupStates.STOPPED);
        }
    },
    goto: {
        label: 'GoTo',
        click: function () {
            StartInterface("hifiapp:GOTO");
            setNotificationState(HifiNotificationType.GOTO, false);
        }
    },
    people: {
        label: 'People',
        click: function () {
            StartInterface("hifiapp:PEOPLE");
            setNotificationState(HifiNotificationType.PEOPLE, false);
        }
    },
    inventory: {
        label: 'Inventory',
        click: function () {
            StartInterface("hifiapp:INVENTORY");
            setNotificationState(HifiNotificationType.ITEMS, false);
            setNotificationState(HifiNotificationType.TRANSACTIONS, false);
        }
    },
    restart: {
        label: 'Start Server',
        click: function() {
            homeServer.restart();
        }
    },
    stopServer: {
        label: 'Stop Server',
        visible: false,
        click: function() {
            homeServer.stop();
        }
    },
    goHome: {
        label: 'Visit Local Server',
        click: visitSandboxClicked,
        enabled: false
    },
    quit: {
        label: 'Quit',
        accelerator: 'Command+Q',
        click: function() {
            shutdown();
        }
    },
    settings: {
        label: 'Settings',
        click: function() {
            shell.openExternal('http://localhost:40100/settings');
        },
        enabled: false
    },
    viewLogs: {
        label: 'View Logs',
        click: function() {
            logWindow.open();
        }
    },
    share: {
        label: 'Share',
        click: function() {
            shell.openExternal('http://localhost:40100/settings/?action=share')
        }
    },
    shuttingDown: {
        label: "Shutting down...",
        enabled: false
    },
}

var separator = {
    type: 'separator'
};


function buildMenuArray(serverState) {

    updateLabels(serverState);

    var menuArray = [];

    if (isShuttingDown) {
        menuArray.push(labels.shuttingDown);
    } else {
        if (isServerInstalled()) {
            menuArray.push(labels.serverState);
            menuArray.push(labels.version);
            menuArray.push(separator);
        }
        if (isServerInstalled() && isInterfaceInstalled()) {
            menuArray.push(labels.goHome);
            menuArray.push(separator);
        }
        if (isServerInstalled()) {
            menuArray.push(labels.restart);
            menuArray.push(labels.stopServer);
            menuArray.push(labels.settings);
            menuArray.push(labels.viewLogs);
            menuArray.push(separator);
        }
        menuArray.push(labels.share);
        menuArray.push(separator);
        if (isInterfaceInstalled()) {
            if (trayNotifications.enabled()) {
                menuArray.push(labels.goto);
                menuArray.push(labels.people);
                menuArray.push(labels.inventory);
                menuArray.push(separator);
            }
            menuArray.push(labels.showNotifications);
            menuArray.push(separator);
        }
        menuArray.push(labels.quit);
    }

    return menuArray;

}

function updateLabels(serverState) {

    // update the tray menu state
    var running = serverState == ProcessGroupStates.STARTED;
    labels.goHome.enabled = running;
    labels.stopServer.visible = running;
    labels.settings.enabled = running;
    if (serverState == ProcessGroupStates.STARTED) {
        labels.serverState.label = "Server - Started";
        labels.restart.label = "Restart Server";
    } else if (serverState == ProcessGroupStates.STOPPED) {
        labels.serverState.label = "Server - Stopped";
        labels.restart.label = "Start Server";
        labels.restart.enabled = true;
    } else if (serverState == ProcessGroupStates.STOPPING) {
        labels.serverState.label = "Server - Stopping";
        labels.restart.label = "Restart Server";
        labels.restart.enabled = false;
    }

    labels.showNotifications.checked = trayNotifications.enabled();
    labels.goto.icon = pendingNotifications[HifiNotificationType.GOTO] ? menuNotificationIcon : null;
    labels.people.icon = pendingNotifications[HifiNotificationType.PEOPLE] ? menuNotificationIcon : null;
    labels.inventory.icon = pendingNotifications[HifiNotificationType.ITEMS] || pendingNotifications[HifiNotificationType.TRANSACTIONS]? menuNotificationIcon : null;
    var onlineUsers = trayNotifications.getOnlineUsers();
    delete labels.people.submenu;
    if (onlineUsers) {
        for (var name in onlineUsers) {
            if(labels.people.submenu == undefined) {
                labels.people.submenu = [];
            }
            labels.people.submenu.push({
                label: name,
                enabled: (onlineUsers[name].location != undefined),
                click: function (item) {
                    setNotificationState(HifiNotificationType.PEOPLE, false);
                    if(onlineUsers[item.label] && onlineUsers[item.label].location) {
                        StartInterface("hifi://" + onlineUsers[item.label].location.root.name + onlineUsers[item.label].location.path);
                    }
                }
            });
        }
    }
    var currentStories = trayNotifications.getCurrentStories();
    delete labels.goto.submenu;
    if (currentStories) {
        for (var location in currentStories) {
            if(labels.goto.submenu == undefined) {
                labels.goto.submenu = [];
            }
            labels.goto.submenu.push({
                label: "event in " + location,
                location: location,
                click: function (item) {
                    setNotificationState(HifiNotificationType.GOTO, false);
                    StartInterface("hifi://" + item.location + currentStories[item.location].path);
                }
            });
        }
    }
}

function updateTrayMenu(serverState) {
    if (tray) {
        var menuArray = buildMenuArray(isShuttingDown ? null : serverState);
        tray.setImage(trayIcons[notificationState]);
        tray.setContextMenu(Menu.buildFromTemplate(menuArray));
        if (isShuttingDown) {
            tray.setToolTip('Vircadia - Shutting Down');
        }
    }
}

const httpStatusPort = 60332;

function backupResourceDirectories(folder) {
    try {
        fs.mkdirSync(folder);
        log.debug("Created directory " + folder);

        var dsBackup = path.join(folder, '/domain-server');
        var acBackup = path.join(folder, '/assignment-client');

        fs.copySync(getDomainServerClientResourcesDirectory(), dsBackup);
        fs.copySync(getAssignmentClientResourcesDirectory(), acBackup);

        fs.removeSync(getDomainServerClientResourcesDirectory());
        fs.removeSync(getAssignmentClientResourcesDirectory());

        return true;
    } catch (e) {
        log.debug(e);
        return false;
    }
}

function removeIncompleteUpdate(acResourceDirectory, dsResourceDirectory) {
    if (fs.existsSync(UPDATER_LOCK_FULL_PATH)) {
        log.debug('Removing incomplete content update files before copying new update');
        fs.emptyDirSync(dsResourceDirectory);
        fs.emptyDirSync(acResourceDirectory);
    } else {
         fs.ensureFileSync(UPDATER_LOCK_FULL_PATH);
    }
}

function maybeInstallDefaultContentSet(onComplete) {
    // Check for existing data
    const acResourceDirectory = getAssignmentClientResourcesDirectory();

    log.debug("Checking for existence of " + acResourceDirectory);

    var userHasExistingACData = true;
    try {
        fs.accessSync(acResourceDirectory);
        log.debug("Found directory " + acResourceDirectory);
    } catch (e) {
        log.debug(e);
        userHasExistingACData = false;
    }

    const dsResourceDirectory = getDomainServerClientResourcesDirectory();

    log.debug("checking for existence of " + dsResourceDirectory);

    var userHasExistingDSData = true;
    try {
        fs.accessSync(dsResourceDirectory);
        log.debug("Found directory " + dsResourceDirectory);
    } catch (e) {
        log.debug(e);
        userHasExistingDSData = false;
    }

    if (userHasExistingACData || userHasExistingDSData) {
        log.debug("User has existing data, suppressing downloader");
        onComplete();

        return;
    }

    log.debug("Found contentPath:" + argv.contentPath);

    if (argv.contentPath) {
        // check if we're updating a data folder whose update is incomplete
        removeIncompleteUpdate(acResourceDirectory, dsResourceDirectory);

        fs.copy(argv.contentPath, getRootHifiDataDirectory(), function (err) {
            if (err) {
                log.debug('Could not copy home content: ' + err);
                return log.error(err)
            }
            log.debug('Copied home content over to: ' + getRootHifiDataDirectory());
            userConfig.set('homeContentLastModified', new Date());
            userConfig.save(configPath);
            fs.removeSync(UPDATER_LOCK_FULL_PATH);
            onComplete();
        });
        return;
    }

    // Show popup
    var window = new BrowserWindow({
        icon: appIcon,
        width: 640,
        height: 480,
        center: true,
        frame: true,
        useContentSize: true,
        resizable: false
    });
    window.loadURL('file://' + __dirname + '/downloader.html');
    if (!debug) {
        window.setMenu(null);
    }
    window.show();

    window.on('closed', onComplete);

    electron.ipcMain.on('ready', function() {
        log.debug("got ready");
        var currentState = '';

        function sendStateUpdate(state, args) {
            // log.debug(state, window, args);
            window.webContents.send('update', { state: state, args: args });
            currentState = state;
        }

        var aborted = false;

        // check if we're updating a data folder whose update is incomplete
        removeIncompleteUpdate(acResourceDirectory, dsResourceDirectory);

        // Start downloading content set
        var req = progress(request.get({
            url: HOME_CONTENT_URL
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
            }
        }), { throttle: 250 }).on('progress', function(state) {
            if (!aborted) {
                // Update progress popup
                sendStateUpdate('downloading', state);
            }
        }).on('end', function(){
            sendStateUpdate('installing');
        });

        function extractError(err) {
            log.debug("Aborting request because gunzip/untar failed");
            aborted = true;
            req.abort();
            log.debug("ERROR" +  err);

            sendStateUpdate('error', {
                message: "Error installing resources."
            });
        }

        var gunzip = zlib.createGunzip();
        gunzip.on('error', extractError);

        req.pipe(gunzip).pipe(tar.extract(getRootHifiDataDirectory())).on('error', extractError).on('finish', function(){
            // response and decompression complete, return
            log.debug("Finished unarchiving home content set");
            userConfig.set('homeContentLastModified', new Date());
            userConfig.save(configPath);
            fs.removeSync(UPDATER_LOCK_FULL_PATH);
            sendStateUpdate('complete');
        });

        window.on('closed', function() {
            if (currentState == 'downloading') {
                req.abort();
            }
        });

        userConfig.set('hasRun', true);
        userConfig.save(configPath);
    });
}

function maybeShowSplash() {
    var suppressSplash = userConfig.get('doNotShowSplash', false);

    if (!suppressSplash) {
        const zoomFactor = 0.8;
        var window = new BrowserWindow({
            icon: appIcon,
            width: 1600 * zoomFactor,
            height: 650 * zoomFactor,
            center: true,
            frame: true,
            useContentSize: true,
            zoomFactor: zoomFactor,
            resizable: false
        });
        window.loadURL('file://' + __dirname + '/splash.html');
        if (!debug) {
            window.setMenu(null);
        }
        window.show();

        window.webContents.on('new-window', function(e, url) {
            e.preventDefault();
            require('shell').openExternal(url);
        });
    }
}


const trayIconOS = (osType == "Darwin") ? "osx" : "win";
var trayIcons = {};
trayIcons[NotificationState.UNNOTIFIED] = "console-tray-" + trayIconOS + ".png";
trayIcons[NotificationState.NOTIFIED] = "console-tray-" + trayIconOS + "-stopped.png";
for (var key in trayIcons) {
    var fullPath = path.join(__dirname, '../resources/' + trayIcons[key]);
    var img = nativeImage.createFromPath(fullPath);
    img.setTemplateImage(osType == 'Darwin');
    trayIcons[key] = img;
}


const notificationIcon = path.join(__dirname, '../resources/console-notification.png');

function isProcessRunning(pid) {
    try {
        // Sending a signal of 0 is effectively a NOOP.
        // If sending the signal is successful, kill will return true.
        // If the process is not running, an exception will be thrown.
        return process.kill(pid, 0);
    } catch (e) {
    }
    return false;
}

function onContentLoaded() {
    // Disable splash window for now.
    // maybeShowSplash();

    if (isServerInstalled()) {
        if (buildInfo.releaseType == 'PRODUCTION' && !argv.noUpdater) {

            const CHECK_FOR_UPDATES_INTERVAL_SECONDS = 60 * 30;
            var hasShownUpdateNotification = false;
            const updateChecker = new updater.UpdateChecker(buildInfo, CHECK_FOR_UPDATES_INTERVAL_SECONDS);
            updateChecker.on('update-available', function(latestVersion, url) {
                if (!hasShownUpdateNotification) {
                    notifier.notify({
                        icon: notificationIcon,
                        title: 'An update is available!',
                        message: 'Vircadia version ' + latestVersion + ' is available',
                        wait: true,
                        appID: buildInfo.appUserModelId,
                        url: url
                    });
                    hasShownUpdateNotification = true;
                }
            });
        }

        deleteOldFiles(logPath, DELETE_LOG_FILES_OLDER_THAN_X_SECONDS, LOG_FILE_REGEX);

        var dsArguments = ['--get-temp-name',
                           '--parent-pid', process.pid];
        domainServer = new Process('domain-server', dsPath, dsArguments, logPath);
        domainServer.restartOnCrash = true;

        var acArguments = ['-n7',
                           '--log-directory', logPath,
                           '--http-status-port', httpStatusPort,
                           '--parent-pid', process.pid];
        acMonitor = new ACMonitorProcess('ac-monitor', acPath, acArguments,
                                         httpStatusPort, logPath);
        acMonitor.restartOnCrash = true;

        homeServer = new ProcessGroup('home', [domainServer, acMonitor]);
        logWindow = new LogWindow(acMonitor, domainServer);

        var processes = {
            home: homeServer
        };

        // handle process updates
        homeServer.on('state-update', function(processGroup) { updateTrayMenu(processGroup.state); });

        // start the home server
        homeServer.start();
    }

    // If we were launched with the launchInterface option, then we need to launch interface now
    if (argv.launchInterface) {
        log.debug("Interface launch requested... argv.launchInterface:", argv.launchInterface);
        StartInterface();
    }

    // If we were launched with the shutdownWith option, then we need to shutdown when that process (pid)
    // is no longer running.
    if (argv.shutdownWith) {
        let pid = argv.shutdownWith;
        console.log("Shutting down with process: ", pid);
        let checkProcessInterval = setInterval(function() {
            let isRunning = isProcessRunning(pid);
            if (!isRunning) {
                log.debug("Watched process is no longer running, shutting down");
                clearTimeout(checkProcessInterval);
                forcedShutdown();
            }
        }, 1000);
    }
}


// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
app.on('ready', function() {

    if (app.dock) {
        // hide the dock icon on OS X
        app.dock.hide();
    }

    // Create tray icon
    tray = new Tray(trayIcons[NotificationState.UNNOTIFIED]);
    tray.setToolTip('Vircadia');

    tray.on('click', function() {
        tray.popUpContextMenu(tray.menu);
    });

    if (isInterfaceInstalled()) {
        trayNotifications.startPolling();
    }
    updateTrayMenu(ProcessGroupStates.STOPPED);

    if (isServerInstalled()) {
        maybeInstallDefaultContentSet(onContentLoaded);
    }
});
