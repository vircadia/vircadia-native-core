'use strict'

const request = require('request');
const extend = require('extend');
const util = require('util');
const events = require('events');
const childProcess = require('child_process');
const fs = require('fs-extra');
const os = require('os');
const path = require('path');

const ProcessGroupStates = {
    STOPPED: 'stopped',
    STARTED: 'started',
    STOPPING: 'stopping'
};

const ProcessStates = {
    STOPPED: 'stopped',
    STARTED: 'started',
    STOPPING: 'stopping'
};



function ProcessGroup(name, processes) {
    events.EventEmitter.call(this);

    this.name = name;
    this.state = ProcessGroupStates.STOPPED;
    this.processes = [];
    this.restarting = false;

    for (let process of processes) {
        this.addProcess(process);
    }
};
util.inherits(ProcessGroup, events.EventEmitter);
ProcessGroup.prototype = extend(ProcessGroup.prototype, {
    addProcess: function(process) {
        this.processes.push(process);
        process.on('state-update', this.onProcessStateUpdate.bind(this));
    },
    start: function() {
        if (this.state != ProcessGroupStates.STOPPED) {
            log.warn("Can't start process group that is not stopped.");
            return;
        }

        for (let process of this.processes) {
            process.start();
        }

        this.state = ProcessGroupStates.STARTED;
        this.emit('state-update', this);
    },
    stop: function() {
        if (this.state != ProcessGroupStates.STARTED) {
            log.warn("Can't stop process group that is not started.");
            return;
        }
        for (let process of this.processes) {
            process.stop();
        }
        this.state = ProcessGroupStates.STOPPING;
        this.emit('state-update', this);
    },
    restart: function() {
        if (this.state == ProcessGroupStates.STOPPED) {
            // start the group, we were already stopped
            this.start();
        } else {
            // set our restart flag so the group will restart once stopped
            this.restarting = true;

            // call stop, that will put them in the stopping state
            this.stop();
        }
    },

    // Event handlers
    onProcessStateUpdate: function(process) {
        var processesStillRunning = false;
        for (let process of this.processes) {
            if (process.state != ProcessStates.STOPPED) {
                processesStillRunning = true;
                break;
            }
        }
        if (!processesStillRunning) {
            this.state = ProcessGroupStates.STOPPED;
            this.emit('state-update', this);

            // if we we're supposed to restart, call start now and reset the flag
            if (this.restarting) {
                this.start();
                this.restarting = false;
            }
        }
        this.emit('process-update', process);
    }
});

var ID = 0;
function Process(name, command, commandArgs, logDirectory) {
    events.EventEmitter.call(this);

    this.id = ++ID;
    this.name = name;
    this.command = command;
    this.commandArgs = commandArgs ? commandArgs : [];
    this.child = null;
    this.logDirectory = logDirectory;
    this.logStdout = null;
    this.logStderr = null;

    this.state = ProcessStates.STOPPED;
};
util.inherits(Process, events.EventEmitter);
Process.prototype = extend(Process.prototype, {
    start: function() {
        if (this.state != ProcessStates.STOPPED) {
            log.warn("Can't start process that is not stopped.");
            return;
        }
        log.debug("Starting " + this.command + " " + this.commandArgs.join(' '));

        var logStdout = 'ignore',
            logStderr = 'ignore';

        if (this.logDirectory) {
            var logDirectoryCreated = false;

            try {
                fs.mkdirsSync(this.logDirectory);
                logDirectoryCreated = true;
            } catch (e) {
                if (e.code == 'EEXIST') {
                    logDirectoryCreated = true;
                } else {
                    log.error("Error creating log directory");
                }
            }

            if (logDirectoryCreated) {
                // Create a temporary file with the current time
                var time = (new Date).getTime();
                var tmpLogStdout = path.resolve(this.logDirectory + '/' + this.name + '-' + time + '-stdout.txt');
                var tmpLogStderr = path.resolve(this.logDirectory + '/' + this.name + '-' + time + '-stderr.txt');

                try {
                    logStdout = fs.openSync(tmpLogStdout, 'ax');
                } catch(e) {
                    log.debug("Error creating stdout log file", e);
                    logStdout = 'ignore';
                }
                try {
                    logStderr = fs.openSync(tmpLogStderr, 'ax');
                } catch(e) {
                    log.debug("Error creating stderr log file", e);
                    logStderr = 'ignore';
                }
            }
        }

        try {
            this.child = childProcess.spawn(this.command, this.commandArgs, {
                detached: false,
                stdio: ['ignore', logStdout, logStderr]
            });
        } catch (e) {
            log.debug("Got error starting child process for " + this.name, e);
            this.child = null;
            this.updateState(ProcessStates.STOPPED);
            return;
        }

        if (logStdout != 'ignore') {
            var pidLogStdout = path.resolve(this.logDirectory + '/' + this.name + "-" + this.child.pid + "-" + time + "-stdout.txt");
            fs.rename(tmpLogStdout, pidLogStdout, function(e) {
                if (e !== null) {
                    log.debug("Error renaming log file from " + tmpLogStdout + " to " + pidLogStdout, e);
                }
            });
            this.logStdout = pidLogStdout;
            fs.closeSync(logStdout);
        }

        if (logStderr != 'ignore') {
            var pidLogStderr = path.resolve(this.logDirectory + '/' + this.name + "-" + this.child.pid + "-" + time + "-stderr.txt");
            fs.rename(tmpLogStderr, pidLogStderr, function(e) {
                if (e !== null) {
                    log.debug("Error renaming log file from " + tmpLogStdout + " to " + pidLogStdout, e);
                }
            });
            this.logStderr = pidLogStderr;

            fs.closeSync(logStderr);
        }

        this.child.on('error', this.onChildStartError.bind(this));
        this.child.on('close', this.onChildClose.bind(this));

        log.debug("Child process started");
        this.updateState(ProcessStates.STARTED);
        this.emit('logs-updated');
    },
    stop: function(force) {
        if (this.state == ProcessStates.STOPPED) {
            log.warn("Can't stop process that is not started or stopping.");
            return;
        }
        if (os.type() == "Windows_NT") {
            var command = "taskkill /pid " + this.child.pid;
            if (force) {
                command += " /f /t";
            }
            childProcess.exec(command, {}, function(error) {
                if (error) {
                    log.error('Error executing taskkill:', error);
                }
            });
        } else {
            var signal = force ? 'SIGKILL' : null;
            this.child.kill(signal);
        }

        log.debug("Stopping child process:", this.child.pid, this.name);

        if (!force) {
            this.stoppingTimeoutID = setTimeout(function() {
                if (this.state == ProcessStates.STOPPING) {
                    log.debug("Force killling", this.name, this.child.pid);
                    this.stop(true);
                }
            }.bind(this), 2500);
        }

        this.updateState(ProcessStates.STOPPING);
    },
    updateState: function(newState) {
        if (this.state != newState) {
            this.state = newState;
            this.emit('state-update', this);
            return true;
        }
        return false;
    },
    getLogs: function() {
        var logs = {};
        logs[this.child.pid] = {
            stdout: this.logStdout == 'ignore' ? null : this.logStdout,
            stderr: this.logStderr == 'ignore' ? null : this.logStderr
        };
        return logs;
    },

    // Events
    onChildStartError: function(error) {
        log.debug("Child process error ", error);
        this.updateState(ProcessStates.STOPPED);
    },
    onChildClose: function(code) {
        log.debug("Child process closed with code ", code, this.name);
        if (this.stoppingTimeoutID) {
            clearTimeout(this.stoppingTimeoutID);
            this.stoppingTimeoutID = null;
        }
        this.updateState(ProcessStates.STOPPED);
    }
});

// ACMonitorProcess is an extension of Process that keeps track of the AC Montior's
// children status and log locations.
const CHECK_AC_STATUS_INTERVAL = 1000;
function ACMonitorProcess(name, path, args, httpStatusPort, logPath) {
    Process.call(this, name, path, args, logPath);

    this.httpStatusPort = httpStatusPort;

    this.requestTimeoutID = null;
    this.pendingRequest = null;
    this.childServers = {};
};
util.inherits(ACMonitorProcess, Process);
ACMonitorProcess.prototype = extend(ACMonitorProcess.prototype, {
    updateState: function(newState) {
        if (ACMonitorProcess.super_.prototype.updateState.call(this, newState)) {
            if (this.state == ProcessStates.STARTED) {
                this._updateACMonitorStatus();
            } else {
                if (this.requestTimeoutID) {
                    clearTimeout(this.requestTimeoutID);
                    this.requestTimeoutID = null;
                }
                if (this.pendingRequest) {
                    this.pendingRequest.destroy();
                    this.pendingRequest = null;
                }
            }
        }
    },
    getLogs: function() {
        var logs = {};
        logs[this.child.pid] = {
            stdout: this.logStdout == 'ignore' ? null : this.logStdout,
            stderr: this.logStderr == 'ignore' ? null : this.logStderr
        };
        for (var pid in this.childServers) {
            logs[pid] = {
                stdout: this.childServers[pid].logStdout,
                stderr: this.childServers[pid].logStderr
            }
        }
        return logs;
    },
    _updateACMonitorStatus: function() {
        if (this.state != ProcessStates.STARTED) {
            return;
        }

        // If there is a pending request, return
        if (this.pendingRequest) {
            return;
        }

        var options = {
            url: "http://localhost:" + this.httpStatusPort + "/status",
            json: true
        };
        this.pendingRequest = request(options, function(error, response, body) {
            this.pendingRequest = null;

            if (error) {
                log.error('ERROR Getting AC Monitor status', error);
            } else {
                this.childServers = body.servers;
            }

            this.emit('logs-updated');

            this.requestTimeoutID = setTimeout(this._updateACMonitorStatus.bind(this), CHECK_AC_STATUS_INTERVAL);
        }.bind(this));
    }
});

module.exports.Process = Process;
module.exports.ACMonitorProcess = ACMonitorProcess;
module.exports.ProcessGroup = ProcessGroup;
module.exports.ProcessGroupStates = ProcessGroupStates;
module.exports.ProcessStates = ProcessStates;
