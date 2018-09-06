const request = require('request');
const extend = require('extend');
const util = require('util');
const events = require('events');
const cheerio = require('cheerio');
const os = require('os');

const platform = os.type() == 'Windows_NT' ? 'windows' : 'mac';

const BUILDS_URL = 'https://highfidelity.com/builds.xml';
const DEV_BUILDS_URL = 'https://highfidelity.com/dev-builds.xml';

// returns 1 if A is greater, 0 if equal, -1 if A is lesser
function semanticVersionCompare(versionA, versionB) {
    var versionAParts = versionA.split('.');
    var versionBParts = versionB.split('.');

    // make sure each version has 3 parts
    var partsLength = versionAParts.length;
    while (partsLength < 3) {
        partsLength = versionAParts.push(0);
    }

    partsLength = versionBParts.length;
    while (partsLength < 3) {
        partsLength = versionBParts.push(0);
    }

    // map all of the parts to numbers
    versionAParts = versionAParts.map(Number);
    versionBParts = versionBParts.map(Number);

    for (var i = 0; i < 3; ++i) {
        if (versionAParts[i] == versionBParts[i]) {
            continue;
        } else if (versionAParts[i] > versionBParts[i]) {
            return 1;
        } else {
            return -1;
        }
    }

    return 0;
}

function UpdateChecker(buildInfo, checkForUpdatesEveryXSeconds) {
    this.stableBuild = (buildInfo.stableBuild == "1");

    this.buildsURL = this.stableBuild ? BUILDS_URL : DEV_BUILDS_URL;
    this.currentVersion = this.stableBuild ? buildInfo.buildIdentifier : parseInt(buildInfo.buildNumber);

    log.debug('Current version is', this.currentVersion);

    setInterval(this.checkForUpdates.bind(this), checkForUpdatesEveryXSeconds * 1000);
    this.checkForUpdates();
};
util.inherits(UpdateChecker, events.EventEmitter);
UpdateChecker.prototype = extend(UpdateChecker.prototype, {
    checkForUpdates: function() {
        log.debug("Checking for updates");
        request(this.buildsURL, (error, response, body) => {
            if (error) {
                log.debug("Error", error);
                return;
            }
            if (response.statusCode == 200) {
                try {
                    var $ = cheerio.load(body, { xmlMode: true });
                    const latestBuild = $('project[name="interface"] platform[name="' + platform + '"]').children().first();

                    var latestVersion = 0;

                    if (this.stableBuild) {
                        latestVersion = latestBuild.find('stable_version').text();
                    } else {
                        latestVersion = parseInt(latestBuild.find('version').text());
                    }

                    log.debug("Latest available update version is:", latestVersion);

                    updateAvailable = false;

                    if (this.stableBuild) {
                        // compare the semantic versions to see if the update is newer
                        updateAvailable = (semanticVersionCompare(latestVersion, this.currentVersion) == 1);
                    } else {
                        // for master builds we just compare the versions as integers
                        updateAvailable = latestVersion > this.currentVersion;
                    }

                    if (updateAvailable) {
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
