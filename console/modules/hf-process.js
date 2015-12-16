'use strict'

var extend = require('extend');
var util = require('util');
var events = require('events');
var childProcess = require('child_process');

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
                detached: false,
                stdio: 'ignore'
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

module.exports.Process = Process;
module.exports.ProcessGroup = ProcessGroup;
module.exports.ProcessStates = ProcessStates;
