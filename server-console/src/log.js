const remote = require('electron').remote;
const os = require('os');
const fs = require('fs');
const Tail = require('always-tail');

function cleanPath(path) {
    if (os.type() == "Windows_NT") {
        // Fix path on Windows
        while (path.indexOf('\\') >= 0) {
            path = path.replace('\\', '/', 'g');
        }
    }
    return path;
}

// a: array, b: array
// Returns: { add: [...], subtract: [...] }
// add: array of items in b but not a
// subtract: array of items in a but not b
function difference(a, b) {
    var add = [];
    var subtract = [];
    for (var k of a) {
        if (b.indexOf(k) == -1) {
            // In a, but not in b
            subtract.push(k);
        }
    }
    for (k of b) {
        if (a.indexOf(k) == -1) {
            // In b, but not in a
            add.push(k);
        }
    }
    return {
        add: add,
        subtract: subtract
    };
}

ready = function() {
    window.$ = require('./vendor/jquery/jquery-2.1.4.min.js');

    var domainServer = remote.getGlobal('domainServer');
    var acMonitor = remote.getGlobal('acMonitor');
    var openLogDirectory = remote.getGlobal('openLogDirectory');

    var pendingLines = {
        'ds': new Array(),
        'ac': new Array()
    };

    var UPDATE_INTERVAL = 16; // Update log at ~60 fps
    var interval = setInterval(flushPendingLines, UPDATE_INTERVAL);

    var logWatchers = {
        'ds': {
        },
        'ac': {
        }
    };

    function updateLogWatchers(stream, watchList, newLogFilesByPID) {
        // Consolidate into a list of the log paths
        var newLogFilePaths = [];
        for (var pid in newLogFilesByPID) {
            newLogFilePaths.push(newLogFilesByPID[pid].stdout);
            newLogFilePaths.push(newLogFilesByPID[pid].stderr);
        }

        var oldLogFilePaths = Object.keys(watchList);
        var diff = difference(oldLogFilePaths, newLogFilePaths);
        // For each removed file, remove it from our watch list
        diff.subtract.forEach(function(removedLogFilePath) {
            watchList[removedLogFilePath].unwatch();
            delete watchList[removedLogFilePath];
            console.log("Unwatching", removedLogFilePath);
        });
        diff.add.forEach(function(addedLogFilePath) {
            const START_AT_X_BYTES_FROM_END = 10000;
            var cleanFilePath = cleanPath(addedLogFilePath);

            // For larger files we won't want to start tailing from the beginning of the file,
            // so start `START_AT_X_BYTES_FROM_END` bytes from the end of the file.
            var start = 0;
            try {
                var fileInfo  = fs.statSync(cleanFilePath);
                start = Math.max(0, fileInfo.size - START_AT_X_BYTES_FROM_END);
            } catch (e) {
                console.error("ERROR READING FILE INFO", e);
            }
            var logTail = new Tail(cleanFilePath, '\n', { start: start, interval: 500 });

            logTail.on('line', function(msg) {
                pendingLines[stream].push(msg);
            });

            logTail.on('error', function(error) {
                console.log("ERROR:", error);
            });

            logTail.watch();

            watchList[addedLogFilePath] = logTail;
            console.log("Watching", cleanFilePath);
        });
    }

    function updateLogFiles() {
        // Get ds and ac logs from main application
        var dsLogs = domainServer.getLogs();
        var acLogs = acMonitor.getLogs();

        updateLogWatchers('ds', logWatchers.ds, dsLogs);
        updateLogWatchers('ac', logWatchers.ac, acLogs);
    }

    window.onbeforeunload = function(e) {
        clearInterval(interval);
        domainServer.removeListener('logs-updated', updateLogFiles);
        acMonitor.removeListener('logs-updated', updateLogFiles);
    };

    domainServer.on('logs-updated', updateLogFiles);
    acMonitor.on('logs-updated', updateLogFiles);


    const maxLogLines = 2500;
    const ipcRenderer = require('electron').ipcRenderer;

    var currentTab = 'domain-server';
    var tabStates = {
        'domain-server': {
            atBottom: true,
            size: 0
        },
        'assignment-client': {
            atBottom: true,
            size: 0
        }
    };
    function setCurrentTab(tabId) {
        if (currentTab == tabId) {
            return;
        }

        var padding = 15;
        $currentTab = $('#' + currentTab);
        tabStates[currentTab].atBottom = $currentTab[0].scrollTop >= ($currentTab[0].scrollHeight - $currentTab.height() - (2 * padding));

        currentTab = tabId;
        $('ul.tabs li').removeClass('current');
        $('.tab-pane').removeClass('current');

        $('li[data-tab=' + tabId + ']').addClass('current');
        var $pidLog = $("#" + tabId);
        $pidLog.addClass('current');

        if (tabStates[tabId].atBottom) {
            $pidLog.scrollTop($pidLog[0].scrollHeight);
        }
    }

    $('ul.tabs li').click(function() {
        setCurrentTab($(this).attr('data-tab'));
    });

    setCurrentTab('domain-server');
    setCurrentTab('assignment-client');

    var filter = "";

    function shouldDisplayLogMessage(message) {
        return !filter || message.toLowerCase().indexOf(filter) >= 0;
    }

    function appendLogMessages(name) {
        var array = pendingLines[name];
        if (array.length === 0) {
            return;
        }
        if (array.length > maxLogLines) {
            array = array.slice(-maxLogLines);
        }

        console.log(name, array.length);

        var id = name == "ds" ? "domain-server" : "assignment-client";
        var $pidLog = $('#' + id);

        var size = tabStates[id].size + array.length;
        if (size > maxLogLines) {
            $pidLog.find('div.log-line:lt(' + (size - maxLogLines) + ')').remove();
        }

        var wasAtBottom = false;
        if (currentTab == id) {
            wasAtBottom = $pidLog[0].scrollTop >= ($pidLog[0].scrollHeight - $pidLog.height());
        }

        for (line in array) {
            var $logLine = $('<div class="log-line">').text(array[line]);
            if (!shouldDisplayLogMessage(array[line])) {
                $logLine.hide();
            }

            $pidLog.append($logLine);
        }

        delete pendingLines[name];
        pendingLines[name] = new Array();

        if (wasAtBottom) {
            $pidLog.scrollTop($pidLog[0].scrollHeight);
        }
    }
    function flushPendingLines() {
        appendLogMessages('ds');
        appendLogMessages('ac');
    }

    // Binding a remote function directly does not work, so bind to a function
    // that calls the remote function.
    $('#view-logs').on('click', function() {
        openLogDirectory();
    });

    // handle filtering of table rows on input change
    $('#search-input').on('input', function() {
        filter = $(this).val().toLowerCase();
        if (filter == "") {
            $('.log-line').show();
        } else {
            $('.log-line').each(function(){
                // decide to hide or show the row if it matches the filter
                if (filter && $(this).text().toLowerCase().indexOf(filter) == -1) {
                    $(this).hide();
                } else {
                    $(this).show();
                }
            });
        }
    });

    updateLogFiles();
};
