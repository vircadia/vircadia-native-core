const request = require('request');
const notifier = require('node-notifier');
const os = require('os');
const process = require('process');
const hfApp = require('./hf-app');
const path = require('path');
const AccountInfo = require('./hf-acctinfo').AccountInfo;
const GetBuildInfo = hfApp.getBuildInfo;
const buildInfo = GetBuildInfo();

const notificationIcon = path.join(__dirname, '../../resources/console-notification.png');
const NOTIFICATION_POLL_TIME_MS = 15 * 1000;
const METAVERSE_SERVER_URL= process.env.HIFI_METAVERSE_URL ? process.env.HIFI_METAVERSE_URL : 'https://metaverse.highfidelity.com'
const STORIES_URL= '/api/v1/user_stories';
const USERS_URL= '/api/v1/users';
const ECONOMIC_ACTIVITY_URL= '/api/v1/commerce/history';
const UPDATES_URL= '/api/v1/commerce/available_updates';
const MAX_NOTIFICATION_ITEMS=30


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
        var text = "";
        var message = "";
        var url = null;
        var app = null;
        switch(this.type) {
            case NotificationType.GOTO:
                text = this.data.username + " " + this.data.action_string + " in " + this.data.place_name;
                message = "Click to go to " + this.data.place_name;
                url = "hifi://" + this.data.place_name + this.data.path;
                break;
            case NotificationType.PEOPLE:
                text = this.data.username + " is available in " + this.data.location.root.name + "!";
                message = "Click to join them.";
                url="hifi://" + this.data.location.root.name + this.data.location.path;
                break;
            case NotificationType.WALLET:
                if(typeof(this.data) == "number") {
                    text = "You have " + this.data + " unread Wallet notifications!";
                    message = "Click to open your wallet."
                    url = "hifiapp:hifi/commerce/wallet/Wallet.qml";
                    break;
                }
                text = "Economic activity.";
                var memo = "";
                if(this.data.sent_certs <= 0 && this.data.received_certs <= 0) {
                    if(this.data.received_money > 0) {
                        text = this.data.sender_name + " sent you " + this.data.received_money + " HFC!";
                        memo = "memo: \"" + this.data.message + "\" ";
                    }
                    else {
                        return;
                    }
                }
                else {
                    text = this.data.message.replace(/<\/?[^>]+(>|$)/g, "");
                }
                message = memo + "Click to open your wallet.";
                url = "hifiapp:hifi/commerce/wallet/Wallet.qml";
                break;
            case NotificationType.MARKETPLACE:
                text = "There's an update available for your version of " + this.data.base_item_title + "!";
                message = "Click to open the marketplace.";
                url = "hifiapp:hifi/commerce/purchases/Purchases.qml";
                break;
        }
        notifier.notify({
            notificationType: this.type,
            icon: notificationIcon,
            title: text,
            message: message,
            wait: true,
            appID: buildInfo.appUserModelId,
            url: url
        });
    }
}

function HifiNotifications(config, callback) {
    this.config = config;
    this.callback = callback;
    this.since = new Date(this.config.get("notifySince", "1970-01-01T00:00:00.000Z"));
    this.enable(this.enabled());
    notifier.on('click', function(notifierObject, options) {
        StartInterface(options.url);
    });
}

HifiNotifications.prototype = {
    enable: function(enabled) {
        this.config.set("enableTrayNotifications", enabled);
        if(enabled) {
            var _this = this;
            this.pollTimer = setInterval(function() {
                var acctInfo = new AccountInfo();
                var token = acctInfo.accessToken(METAVERSE_SERVER_URL);
                var _since = _this.since;
                _this.since = new Date();
                IsInterfaceRunning(function(running) {
                    if(running) {
                        return;
                    }
                    _this.pollForStories(_since, token);
                    _this.pollForConnections(_since, token);
                    _this.pollForEconomicActivity(_since, token);
                    _this.pollForMarketplaceUpdates(_since, token);
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
    _pollCommon: function(notifyType, error, data, since) {
        var maxNotificationItemCount = (since.getTime() == 0) ? MAX_NOTIFICATION_ITEMS : 1;
        if (error || !data.body) {
            console.log("Error: unable to get " + url);
            return false;
        }
        var content = JSON.parse(data.body);
        if(!content || content.status != 'success') {
            console.log("Error: unable to get " + url);
            return false;
        }
        console.log(content);
        if(!content.total_entries) {
            return;
        }
        this.callback(notifyType, true);
        if(content.total_entries >= maxNotificationItemCount) {
            var notification = new HifiNotification(notifyType, content.total_entries);
            notification.show();   
        }
        else {
            var notifyData = []
            switch(notifyType) {
                case NotificationType.GOTO:
                    notifyData = content.user_stories;
                    break;
                case NotificationType.PEOPLE:
                    notifyData = content.data.users;
                    break;
                case NotificationType.WALLET:
                    notifyData = content.data.history;
                    break;
                case NotificationType.MARKETPLACE:
                    notifyData = content.data.updates;
                    break;
            }

            notifyData.forEach(function(data) {
                var notification = new HifiNotification(notifyType, data);
                notification.show();
            });
        }
    },
    pollForStories: function(since, token) {
        var _this = this;
        var actions = 'announcement';
        var options = [
            'now=' + new Date().toISOString(),
            'since=' + since.getTime() / 1000,
            'include_actions=announcement',
            'restriction=open,hifi',
            'require_online=true',
            'per_page='+MAX_NOTIFICATION_ITEMS
        ];
        console.log("Polling for stories");
        var url = METAVERSE_SERVER_URL + STORIES_URL + '?' + options.join('&');
        console.log(url);
        request({
            uri: url
        }, function (error, data) {
            _this._pollCommon(NotificationType.GOTO, error, data, since);
        });
    },
    pollForConnections: function(since, token) {
        var _this = this;
        var options = [
            'filter=connections',
            'since=' + since.getTime() / 1000,
            'status=online',
            'page=1',
            'per_page=' + MAX_NOTIFICATION_ITEMS
        ];
        console.log("Polling for connections");
        var url = METAVERSE_SERVER_URL + USERS_URL + '?' + options.join('&');
        console.log(url);
        request.get({
            uri: url,
            'auth': {
                'bearer': token
              }
        }, function (error, data) {
            _this._pollCommon(NotificationType.PEOPLE, error, data, since);
        });
    },
    pollForEconomicActivity: function(since, token) {
        var _this = this;
        var options = [
            'since=' + since.getTime() / 1000,
            'page=1',
            'per_page=' + 1000 // total_entries is incorrect for wallet queries if results
                               // aren't all on one page, so grab them all on a single page
                               // for now.
        ];
        console.log("Polling for economic activity");
        var url = METAVERSE_SERVER_URL + ECONOMIC_ACTIVITY_URL + '?' + options.join('&');
        console.log(url);
        request.post({
            uri: url,
            'auth': {
                'bearer': token
              }
        }, function (error, data) {
            _this._pollCommon(NotificationType.WALLET, error, data, since);
        });
    },
    pollForMarketplaceUpdates: function(since, token) {
        var _this = this;
        var options = [
            'since=' + since.getTime() / 1000,
            'page=1',
            'per_page=' + MAX_NOTIFICATION_ITEMS
        ];
        console.log("Polling for marketplace update");
        var url = METAVERSE_SERVER_URL + UPDATES_URL + '?' + options.join('&');
        console.log(url);
        request.put({
            uri: url,
            'auth': {
                'bearer': token
              }
        }, function (error, data) {
            _this._pollCommon(NotificationType.MARKETPLACE, error, data, since);
        });
    }
};

exports.HifiNotifications = HifiNotifications;
exports.NotificationType = NotificationType;