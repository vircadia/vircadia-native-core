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

ready = function() {
    window.$ = require('./vendor/jquery/jquery-2.1.4.min.js');

    console.log(remote.debug);

    var domainServer = remote.getGlobal('domainServer');
    var acMonitor = remote.getGlobal('acMonitor');

    var logFiles = {
        'ds': {
        },
        'ac': {
        }
    };

    function updateLogFiles() {
        var dsLogs = domainServer.getLogs();
        var acLogs = acMonitor.getLogs();

        // Update ds logs
        var dsLogFilePaths = [];
        for (var pid in dsLogs) {
            dsLogFilePaths.push(dsLogs[pid].stdout);
            dsLogFilePaths.push(dsLogs[pid].stderr);
        }
        console.log(dsLogFilePaths);
        console.log(dsLogs);
        var dsFilePaths = Object.keys(logFiles.ds);
        for (const filePath of dsFilePaths) {
            if (dsLogFilePaths.indexOf(filePath) == -1) {
                // This file is no longer being used, let's stop watching it
                // and remove it from our list.
                logFiles.ds[filePath].unwatch();
                delete logFiles[filePath];
            }
        }
        dsLogFilePaths.forEach(function(filePath) {
            if (logFiles.ds[filePath] === undefined) {
                var cleanFilePath = cleanPath(filePath);

                var logTail = new Tail(cleanFilePath, '\n', { start: 0, interval: 500 });

                logTail.on('line', function(msg) {
                    console.log('msg', msg, 'ds');
                    appendLogMessage(0, msg, 'ds');
                });

                logTail.on('error', function(error) {
                    console.log("ERROR:", error);
                });

                logTail.watch();

                logFiles.ds[filePath] = logTail;
                console.log("Watching", cleanFilePath);
            }
        });

        // Update ac logs
        var acLogFilePaths = [];
        for (var pid in acLogs) {
            acLogFilePaths.push(acLogs[pid].stdout);
            acLogFilePaths.push(acLogs[pid].stderr);
        }
        console.log(acLogFilePaths);
        console.log(acLogs);
        var acFilePaths = Object.keys(logFiles.ac);
        for (filePath of acFilePaths) {
            if (acLogFilePaths.indexOf(filePath) == -1) {
                // This file is no longer being used, let's stop watching it
                // and remove it from our list.
                logFiles.ac[filePath].unwatch();
                delete logFiles[filePath];
            }
        }
        acLogFilePaths.forEach(function(filePath) {
            if (logFiles.ac[filePath] === undefined) {
                var cleanFilePath = cleanPath(filePath);

                var fs = require('fs');

                var stats = fs.statSync(cleanFilePath);
                var size = stats.size;
                var start = Math.max(0, size - (25000));
                console.log(cleanFilePath, size, start, maxLogLines);

                var logTail = new Tail(cleanFilePath, '\n', { start: start, interval: 500 });

                logTail.on('line', function(msg) {
                    console.log('msg', msg, 'ac');
                    appendLogMessage(0, msg, 'ac');
                });

                logTail.on('error', function(error) {
                    console.log("ERROR:", error);
                });

                logTail.watch();

                logFiles.ac[filePath] = logTail;
                console.log("Watching", cleanFilePath);
            }
        });

        console.log(dsLogs, acLogs);
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

    // Register for log events
    //   Process added

    function shouldDisplayLogMessage(message) {
        return !filter || message.toLowerCase().indexOf(filter) >= 0;
    }

    function appendLogMessage(pid, msg, name) {
        console.log(pid, msg, name);
        var id = "pid-" + pid;
        id = name == "ds" ? "domain-server" : "assignment-client";
        var $pidLog = $('#' + id);

        var size = ++tabStates[id].size;
        if (size > maxLogLines) {
            // $logLines.first().remove();
            $pidLog.find('div.log-line:first').remove();
            removed = true;
        }

        var wasAtBottom = false;
        if (currentTab == id) {
            var padding = 15;
            wasAtBottom = $pidLog[0].scrollTop >= ($pidLog[0].scrollHeight - $pidLog.height() - (2 * padding));
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
