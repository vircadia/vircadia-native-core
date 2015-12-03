'use strict'

var extend = require('extend');
var util = require('util');
var events = require('events');
var electron = require('electron');
var app = electron.app;  // Module to control application life.
var BrowserWindow = require('browser-window');  // Module to create native browser window.
var childProcess = require('child_process');

const ipcMain = electron.ipcMain;

// Report crashes to our server.
require('crash-reporter').start();

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the JavaScript object is garggbage collected.
var mainWindow = null;

const ProcessStates = {
    STOPPED: 'stopped',
    STARTED: 'started',
    STOPPING: 'stopping'
};

function ProcessGroup(name, processes) {
    this.name = name;
    this.processes = processes;
};
util.inherits(ProcessGroup, events.EventEmitter);
ProcessGroup.prototype = extend(ProcessGroup.prototype, {
    addProcess: function(process) {
        this.processes.push(process);
    },
    start: function() {
        for (let process of this.processes) {
            process.start();
        }
    },
    stop: function() {
        for (let process of this.processes) {
            process.stop();
        }
    }
});

var ID = 0;
function Process(name, command, commandArgs) {
    events.EventEmitter.call(this);

    this.id = ++ID;
    this.name = name;
    this.command = command;
    this.commandArgs = commandArgs ? commandArgs : [];
    this.child = null;

    this.state = ProcessStates.STOPPED;
};
util.inherits(Process, events.EventEmitter);
Process.prototype = extend(Process.prototype, {
    start: function() {
        if (this.state != ProcessStates.STOPPED) {
            console.warn("Can't start process that is not stopped.");
            return;
        }
        console.log("Starting " + this.command);
        try {
            this.child = childProcess.spawn(this.command, this.commandArgs, {
                detached: false
            });
            //console.log("started ", this.child);
            this.child.on('error', this.onChildStartError.bind(this));
            this.child.on('close', this.onChildClose.bind(this));
            this.state = ProcessStates.STARTED;
            console.log("Child process started");
        } catch (e) {
            console.log("Got error starting child process for " + this.name, e);
            this.child = null;
            this.state = ProcessStates.STOPPED;
        }

        this.emit('state-update');
    },
    stop: function() {
        if (this.state != ProcessStates.STARTED) {
            console.warn("Can't stop process that is not started.");
            return;
        }
        this.child.kill();
        this.state = ProcessStates.STOPPING;
    },

    // Events
    onChildStartError: function(error) {
        console.log("Child process error ", error);
        this.state = ProcessStates.STOPPED;
        this.emit('state-update');
    },
    onChildClose: function(code) {
        console.log("Child process closed with code ", code);
        this.state = ProcessStates.STOPPED;
        this.emit('state-update');
    }
});

// Quit when all windows are closed.
app.on('window-all-closed', function() {
    // On OS X it is common for applications and their menu bar
    // to stay active until the user quits explicitly with Cmd + Q
    if (process.platform != 'darwin') {
        app.quit();
    }
});

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
app.on('ready', function() {
    // Create the browser window.
    mainWindow = new BrowserWindow({width: 800, height: 600});

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
