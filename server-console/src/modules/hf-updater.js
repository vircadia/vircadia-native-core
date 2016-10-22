const request = require('request');
const extend = require('extend');
const util = require('util');
const events = require('events');
const cheerio = require('cheerio');
const os = require('os');
const log = require('electron-log');

const platform = os.type() == 'Windows_NT' ? 'windows' : 'mac';

const BUILDS_URL = 'https://highfidelity.com/builds.xml';

function UpdateChecker(currentVersion, checkForUpdatesEveryXSeconds) {
    this.currentVersion = currentVersion;
    log.debug('cur', currentVersion);

    setInterval(this.checkForUpdates.bind(this), checkForUpdatesEveryXSeconds * 1000);
    this.checkForUpdates();
};
util.inherits(UpdateChecker, events.EventEmitter);
UpdateChecker.prototype = extend(UpdateChecker.prototype, {
    checkForUpdates: function() {
        log.debug("Checking for updates");
        request(BUILDS_URL, (error, response, body) => {
            if (error) {
                log.debug("Error", error);
                return;
            }
            if (response.statusCode == 200) {
                try {
                    var $ = cheerio.load(body, { xmlMode: true });
                    const latestBuild = $('project[name="interface"] platform[name="' + platform + '"]').children().first();
                    const latestVersion = parseInt(latestBuild.find('version').text());
                    log.debug("Latest version is:", latestVersion, this.currentVersion);
                    if (latestVersion > this.currentVersion) {
                        const url = latestBuild.find('url').text();
                        this.emit('update-available', latestVersion, url);
                    }
                } catch (e) {
                    log.warn("Error when checking for updates", e);
                }
            }
        });
    }
});

exports.UpdateChecker = UpdateChecker;
