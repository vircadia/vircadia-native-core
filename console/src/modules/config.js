var fs = require('fs');
var extend = require('extend');

function Config() {
    this.data = {};
}
Config.prototype = {
    load: function(filePath) {
        var rawData = null;
        try {
            rawData = fs.readFileSync(filePath);
        } catch(e) {
            console.log("Config file not found");
        }
        var configData = {};

        try {
            if (rawData) {
                configData = JSON.parse(rawData);
            } else {
                configData = {};
            }
        } catch(e) {
            console.error("Error parsing config file", filePath)
        }

        this.data = {};
        extend(true, this.data, configData);
    },
    save: function(filePath) {
        fs.writeFileSync(filePath, JSON.stringify(this.data));
    },
    get: function(key, defaultValue) {
        if (this.data.hasOwnProperty(key)) {
            return this.data[key];
        }
        return defaultValue;
    },
    set: function(key, value) {
        console.log("Setting", key, "to", value);
        this.data[key] = value;
    }
};

exports.Config = Config;
