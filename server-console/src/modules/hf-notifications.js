const request = require('request');
const notifier = require('node-notifier');
const os = require('os');
const process = require('process');
const hfApp = require('./hf-app');
const path = require('path');

const notificationIcon = path.join(__dirname, '../../resources/console-notification.png');
const NOTIFICATION_POLL_TIME_MS = 15 * 1000;
const METAVERSE_SERVER_URL= process.env.HIFI_METAVERSE_URL ? process.env.HIFI_METAVERSE_URL : 'https://highfidelity.com'
const STORIES_URL= '/api/v1/user_stories';
const USERS_URL= '/api/v1/users';
const ECONOMIC_ACTIVITY_URL= '/api/v1/commerce/history';
const UPDATES_URL= '/api/v1/commerce/available_updates';


const StartInterface=hfApp.startInterface;
const IsInterfaceRunning=hfApp.isInterfaceRunning;

const NotificationType = {
    GOTO:        'goto',
    PEOPLE:      'people',
    WALLET:      'wallet',
    MARKETPLACE: 'marketplace'
};

function HifiNotification(notificationType, notificationData) {
    this.type = notificationType;
    this.data = notificationData;
}

HifiNotification.prototype = {
    show: function() {
        switch(this.type) {
            case NotificationType.GOTO:
                var text = this.data.username + " " + this.data.action_string + " in " + this.data.place_name;
                notifier.notify({
                    notificationType: this.type,
                    icon: notificationIcon,
                    title: text,
                    message: "Click to goto " + this.data.place_name,
                    wait: true,
                    url: "hifi://" + this.data.place_name + this.data.path
                });
                break;

            case NotificationType.PEOPLE:
                var text = this.data.username + " has logged in.";
                notifier.notify({
                    notificationType: this.type,
                    icon: notificationIcon,
                    title: text,
                    message: "Click to join them in " + this.data.location.root.name,
                    wait: true,
                    url: "hifi://" + this.data.location.root.name + this.data.location.path
                });
                break;

            case NotificationType.WALLET:
                var text = "Economic activity.";
                notifier.notify({
                    notificationType: this.type,
                    icon: notificationIcon,
                    title: text,
                    message: "Click to open your wallet",
                    wait: true,
                    app: "Wallet"
                });
                break;

            case NotificationType.MARKETPLACE:
                var text = "One of your marketplace items has an update.";
                notifier.notify({
                    notificationType: this.type,
                    icon: notificationIcon,
                    title: text,
                    message: "Click to start the marketplace app",
                    wait: true,
                    app: "Marketplace"
                });
                break;
        }
    }
}

function HifiNotifications(config, callback) {
    this.config = config;
    this.callback = callback;
    this.since = new Date(this.config.get("notifySince", "1970-01-01T00:00:00.000Z"));
    this.enable(this.enabled());
    notifier.on('click', function(notifierObject, options) {
        console.log(options);
        StartInterface(options.url);
    });
}

HifiNotifications.prototype = {
    enable: function(enabled) {
        this.config.set("enableTrayNotifications", enabled);
        if(enabled) {
            var _this = this;
            this.pollTimer = setInterval(function() {
                var _since = _this.since;
                _this.since = new Date();
                IsInterfaceRunning(function(running) {
                    if(running) {
                        return;
                    }
                    _this.pollForStories(_since, _this.callback);
                    _this.pollForConnections(_since, _this.callback);
                    _this.pollForEconomicActivity(_since, _this.callback);
                    _this.pollForMarketplaceUpdates(_since, _this.callback);
                });
            },
            NOTIFICATION_POLL_TIME_MS);
        }
        else if(this.pollTimer) {
            clearInterval(this.pollTimer);
        }
    },
    enabled: function() {
        return this.config.get("enableTrayNotifications", true);
    },
    stopPolling: function() {
        this.config.set("notifySince", this.since.toISOString());
        if(this.pollTimer) {
            clearInterval(this.pollTimer);
        }
    },
    pollForStories: function(since, callback) {
        var _this = this;
        var actions = 'announcement';
        var options = [
            'now=' + new Date().toISOString(),
            'since=' + since.toISOString(),
            'include_actions=announcement',
            'restriction=open,hifi',
            'require_online=true'
        ];
        console.log("Polling for stories");
        var url = METAVERSE_SERVER_URL + STORIES_URL + '?' + options.join('&');
        request({
            uri: url
        }, function (error, data) {
            if (error || !data.body) {
                console.log("Error: unable to get " + url);
                return;
            }
            var content = JSON.parse(data.body);
            if(!content || content.status != 'success') {
                console.log("Error: unable to get " + url);
                return;
            }
            content.user_stories.forEach(function(story) {
                var updated_at = new Date(story.updated_at);
                if (updated_at < since) {
                    return;
                }
                callback(NotificationType.GOTO);
                var notification = new HifiNotification(NotificationType.GOTO, story);
                notification.show();
            });
        });
    },
    pollForConnections: function(since, callback) {
        var _this = this;
        var options = [
            'filter=connections',
            'since=' + since.toISOString(),
            'status=online'
        ];
        console.log("Polling for connections");
        var url = METAVERSE_SERVER_URL + USERS_URL + '?' + options.join('&');
        request({
            uri: url
        }, function (error, data) {
            if (error || !data.body) {
                console.log("Error: unable to get " + url);
                return;
            }
            var content = JSON.parse(data.body);
            if(!content || content.status != 'success') {
                console.log("Error: unable to get " + url);
                return;
            }
            console.log(content.data);
            content.data.users.forEach(function(user) {
                if(user.online) {
                    callback(NotificationType.PEOPLE);
                    var notification = new HifiNotification(NotificationType.PEOPLE, user);
                    notification.show();
                }
            });
        });
    },
    pollForEconomicActivity: function(since, callback) {
        var _this = this;
        var options = [
            'filter=connections',
            'since=' + since.toISOString(),
            'status=online'
        ];
        console.log("Polling for economic activity");
        var url = METAVERSE_SERVER_URL + ECONOMIC_ACTIVITY_URL + '?' + options.join('&');
        request.post({
            uri: url
        }, function (error, data) {
            if (error || !data.body) {
                console.log("Error " + error + ": unable to post " + url);
                console.log(data);
                return;
            }
            var content = JSON.parse(data.body);
            if(!content || content.status != 'success') {
                console.log(data.body);
                console.log("Error " + content.status + ": unable to post " + url);
                return;
            }
            console.log(content.data);
            content.data.users.forEach(function(user) {
                if(user.online) {
                    callback(NotificationType.PEOPLE);
                    var notification = new HifiNotification(NotificationType.PEOPLE, user);
                    notification.show();
                }
            });
        });
    },
    pollForMarketplaceUpdates: function(since, callback) {
        var _this = this;
        var options = [
            'filter=connections',
            'since=' + since.toISOString(),
            'status=online'
        ];
        console.log("Polling for marketplace update");
        var url = METAVERSE_SERVER_URL + UPDATES_URL + '?' + options.join('&');
        request.put({
            uri: url
        }, function (error, data) {
            if (error || !data.body) {
                console.log("Error " + error + ": unable to put " + url);
                return;
            }
            var content = JSON.parse(data.body);
            if(!content || content.status != 'success') {
                console.log(data.body);
                console.log("Error " + content.status + ": unable to put " + url);
                return;
            }
            content.data.users.forEach(function(user) {
                if(user.online) {
                    callback(NotificationType.PEOPLE);
                    var notification = new HifiNotification(NotificationType.PEOPLE, user);
                    notification.show();
                }
            });
        });
    }
};

exports.HifiNotifications = HifiNotifications;
exports.NotificationType = NotificationType;