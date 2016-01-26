var remote = require('electron').remote;
var os = require('os');
var Tail = require('always-tail');

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
            // In a, but not in b
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
        console.log('diff', diff);
        // For each removed file, remove it from our watch list
        diff.subtract.forEach(function(removedLogFilePath) {
            watchList[removedLogFilePath].unwatch();
            delete watchList[removedLogFilePath];
        });
        diff.add.forEach(function(addedLogFilePath) {
            var cleanFilePath = cleanPath(addedLogFilePath);

            var logTail = new Tail(cleanFilePath, '\n', { start: 0, interval: 500 });

            logTail.on('line', function(msg) {
                appendLogMessage(0, msg, stream);
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

    $('ul.tabs li').click(function(){
        setCurrentTab($(this).attr('data-tab'));
    });

    setCurrentTab('domain-server');
    setCurrentTab('assignment-client');

    var filter = "";

    function shouldDisplayLogMessage(message) {
        return !filter || message.toLowerCase().indexOf(filter) >= 0;
    }

    function appendLogMessage(pid, msg, name) {
        var id = name == "ds" ? "domain-server" : "assignment-client";
        var $pidLog = $('#' + id);

        var size = ++tabStates[id].size;
        if (size > maxLogLines) {
            $pidLog.find('div.log-line:first').remove();
            removed = true;
        }

        var wasAtBottom = false;
        if (currentTab == id) {
            wasAtBottom = $pidLog[0].scrollTop >= ($pidLog[0].scrollHeight - $pidLog.height());
        }

        var $logLine = $('<div class="log-line">').text(msg);
        if (!shouldDisplayLogMessage(msg)) {
            $logLine.hide();
        }

        $pidLog.append($logLine);

        if (wasAtBottom) {
            $pidLog.scrollTop($pidLog[0].scrollHeight);
        }

    }

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
