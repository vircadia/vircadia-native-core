const fs = require('fs');
const extend = require('extend');
const Config = require('./config').Config
const os = require('os');
const pathFinder = require('./path-finder');
const path = require('path');
const argv = require('yargs').argv;
const hfprocess = require('./hf-process');
const osHomeDir = require('os-homedir');
const Process = hfprocess.Process;

const binaryType = argv.binaryType;
const osType = os.type();

exports.getBuildInfo = function() {
    var buildInfoPath = null;

    if (osType == 'Windows_NT') {
        buildInfoPath = path.join(path.dirname(process.execPath), 'build-info.json');
    } else if (osType == 'Darwin') {
        var contentPath = ".app/Contents/";
        var contentEndIndex = __dirname.indexOf(contentPath);

        if (contentEndIndex != -1) {
            // this is an app bundle
            var appPath = __dirname.substring(0, contentEndIndex) + ".app";
            buildInfoPath = path.join(appPath, "/Contents/Resources/build-info.json");
        }
    }

    const DEFAULT_BUILD_INFO = {
        releaseType: "",
        buildIdentifier: "dev",
        buildNumber: "0",
        stableBuild: "0",
        organization: "High Fidelity - dev",
        appUserModelId: "com.highfidelity.sandbox-dev"
    };
    var buildInfo = DEFAULT_BUILD_INFO;

    if (buildInfoPath) {
        try {
            buildInfo = JSON.parse(fs.readFileSync(buildInfoPath));
        } catch (e) {
            buildInfo = DEFAULT_BUILD_INFO;
        }
    }

    return buildInfo;
}

const buildInfo = exports.getBuildInfo();
const interfacePath = pathFinder.discoveredPath("Interface", binaryType, buildInfo.releaseType);

exports.startInterface = function(url) {
    var argArray = [];

    // check if we have a url parameter to include
    if (url) {
        argArray = ["--url", url];
    }
    console.log("Starting with " + url);
    // create a new Interface instance - Interface makes sure only one is running at a time
    var pInterface = new Process('Interface', interfacePath, argArray);
    pInterface.detached = true;
    pInterface.start();
}

exports.isInterfaceRunning = function(done) {
    var pInterface = new Process('interface', 'interface.exe');
    return pInterface.isRunning(done);
}


exports.getRootHifiDataDirectory = function(local) {
    var organization = buildInfo.organization;
    if (osType == 'Windows_NT') {
        if (local) {
            return path.resolve(osHomeDir(), 'AppData/Local', organization);
        } else {
            return path.resolve(osHomeDir(), 'AppData/Roaming', organization);
        }
    } else if (osType == 'Darwin') {
        return path.resolve(osHomeDir(), 'Library/Application Support', organization);
    } else {
        return path.resolve(osHomeDir(), '.local/share/', organization);
    }
}

exports.getDomainServerClientResourcesDirectory = function() {
    return path.join(exports.getRootHifiDataDirectory(), '/domain-server');
}

exports.getAssignmentClientResourcesDirectory = function()  {
    return path.join(exports.getRootHifiDataDirectory(), '/assignment-client');
}

exports.getApplicationDataDirectory = function(local) {
    return path.join(exports.getRootHifiDataDirectory(local), '/Server Console');
}

exports.getInterfaceDataDirectory = function(local) {
    return path.join(exports.getRootHifiDataDirectory(local), '/Interface');
}