const request = require('request');
const notifier = require('node-notifier');
const os = require('os');
const process = require('process');
const hfApp = require('./hf-app');
const path = require('path');
const AccountInfo = require('./hf-acctinfo').AccountInfo;
const url = require('url');
const shell = require('electron').shell;
const GetBuildInfo = hfApp.getBuildInfo;
const buildInfo = GetBuildInfo();
const osType = os.type();

const notificationIcon = path.join(__dirname, '../../resources/console-notification.png');
const STORIES_NOTIFICATION_POLL_TIME_MS = 120 * 1000;
const PEOPLE_NOTIFICATION_POLL_TIME_MS = 120 * 1000;
const WALLET_NOTIFICATION_POLL_TIME_MS = 600 * 1000;
const MARKETPLACE_NOTIFICATION_POLL_TIME_MS = 600 * 1000;
const OSX_CLICK_DELAY_TIMEOUT = 500;


const METAVERSE_SERVER_URL= process.env.HIFI_METAVERSE_URL ? process.env.HIFI_METAVERSE_URL : 'https://metaverse.highfidelity.com'
const STORIES_URL= '/api/v1/user_stories';
const USERS_URL= '/api/v1/users';
const ECONOMIC_ACTIVITY_URL= '/api/v1/commerce/history';
const UPDATES_URL= '/api/v1/commerce/available_updates';
const MAX_NOTIFICATION_ITEMS=30
const STARTUP_MAX_NOTIFICATION_ITEMS=1


const StartInterface=hfApp.startInterface;
const IsInterfaceRunning=hfApp.isInterfaceRunning;

const NotificationType = {
    GOTO:        'goto',
    PEOPLE:      'people',
    WALLET:      'wallet',
    MARKETPLACE: 'marketplace'
};


function HifiNotification(notificationType, notificationData, menuNotificationCallback) {
    this.type = notificationType;
    this.data = notificationData;
}

HifiNotification.prototype = {
    show: function (finished) {
        var text = "";
        var message = "";
        var url = null;
        var app = null;
        switch (this.type) {
            case NotificationType.GOTO:
                if (typeof(this.data) === "number") {
                    if (this.data === 1) {
                        text = "You have " + this.data + " event invitation pending."
                    } else {
                        text = "You have " + this.data + " event invitations pending."
                    }
                    message = "Click to open GOTO.";
                    url="hifiapp:GOTO"
                } else {
                    text = this.data.username + " " + this.data.action_string + " in " + this.data.place_name + ".";
                    message = "Click to go to " + this.data.place_name + ".";
                    url = "hifi://" + this.data.place_name + this.data.path;
                }
                break;

            case NotificationType.PEOPLE:
                if (typeof(this.data) === "number") {
                    if (this.data === 1) {
                        text = this.data + " of your connections is online."
                    } else {
                        text = this.data + " of your connections are online."
                    }
                    message = "Click to open PEOPLE.";
                    url="hifiapp:PEOPLE";
                } else {
                    if (this.data.location) {
                        text = this.data.username + " is available in " + this.data.location.root.name + ".";
                        message = "Click to join them.";
                        url="hifi://" + this.data.location.root.name + this.data.location.path;
                    } else {
                        text = this.data.username + " is online.";
                        message = "Click to open PEOPLE.";
                        url="hifiapp:PEOPLE";
                    }
                }
                break;

            case NotificationType.WALLET:
                if (typeof(this.data) === "number") {
                    if (this.data === 1) {
                        text = "You have " + this.data + " unread Wallet transaction.";
                    } else {
                        text = "You have " + this.data + " unread Wallet transactions.";
                    }
                    message = "Click to open WALLET."
                    url = "hifiapp:hifi/commerce/wallet/Wallet.qml";
                    break;
                }
                text = this.data.message.replace(/<\/?[^>]+(>|$)/g, "");
                message = "Click to open WALLET.";
                url = "hifiapp:WALLET";
                break;

            case NotificationType.MARKETPLACE:
                if (typeof(this.data) === "number") {
                    if (this.data === 1) {
                        text = this.data + " of your purchased items has an update available.";
                    } else {  
                        text = this.data + " of your purchased items have updates available.";
                    }
                } else {
                    text = "Update available for " + this.data.base_item_title + ".";
                }
                message = "Click to open MARKET.";
                url = "hifiapp:MARKET";
                break;
        }
        notifier.notify({
            notificationType: this.type,
            icon: notificationIcon,
            title: text,
            message: message,
            wait: true,
            appID: buildInfo.appUserModelId,
            url: url,
            timeout: 5
            },
            function (error, reason, metadata) {
                if (finished) {
                    if (osType === 'Darwin') {
                        setTimeout(finished, OSX_CLICK_DELAY_TIMEOUT);
                    } else {
                        finished();
                    }
                }
            });
    }
}

function HifiNotifications(config, menuNotificationCallback) {
    this.config = config;
    this.menuNotificationCallback = menuNotificationCallback;
    this.onlineUsers = {};
    this.currentStories = {};
    this.storiesSince = new Date(this.config.get("storiesNotifySince", "1970-01-01T00:00:00.000Z"));
    this.peopleSince = new Date(this.config.get("peopleNotifySince", "1970-01-01T00:00:00.000Z"));
    this.walletSince = new Date(this.config.get("walletNotifySince", "1970-01-01T00:00:00.000Z"));
    this.marketplaceSince = new Date(this.config.get("marketplaceNotifySince", "1970-01-01T00:00:00.000Z"));  

    this.pendingNotifications = [];


    var _menuNotificationCallback = menuNotificationCallback;
    notifier.on('click', function (notifierObject, options) {
        const optUrl = url.parse(options.url);
        if ((optUrl.protocol === "hifi:") || (optUrl.protocol === "hifiapp:")) {
            StartInterface(options.url);
            _menuNotificationCallback(options.notificationType, false);
        } else {
            shell.openExternal(options.url);
        }
    });
}

HifiNotifications.prototype = {
    enable: function (enabled) {
        this.config.set("disableTrayNotifications", !enabled);
        if (enabled) {
            var _this = this;
            this.storiesPollTimer = setInterval(function () {
                var _since = _this.storiesSince;
                _this.storiesSince = new Date();
                _this.pollForStories(_since);
            },
            STORIES_NOTIFICATION_POLL_TIME_MS);

            this.peoplePollTimer = setInterval(function () {
                var _since = _this.peopleSince;
                _this.peopleSince = new Date();
                _this.pollForConnections(_since);
            },
            PEOPLE_NOTIFICATION_POLL_TIME_MS);

            this.walletPollTimer = setInterval(function () {
                var _since = _this.walletSince;
                _this.walletSince = new Date();
                _this.pollForEconomicActivity(_since);
            },
            WALLET_NOTIFICATION_POLL_TIME_MS);

            this.marketplacePollTimer = setInterval(function () {
                var _since = _this.marketplaceSince;
                _this.marketplaceSince = new Date();
                _this.pollForMarketplaceUpdates(_since);
            },
            MARKETPLACE_NOTIFICATION_POLL_TIME_MS);
        } else {
            this.stopPolling();
        }
    },
    enabled: function () {
        return !this.config.get("disableTrayNotifications", false);
    },
    startPolling: function () {
        this.enable(this.enabled());
    },
    stopPolling: function () {
        this.config.set("storiesNotifySince", this.storiesSince.toISOString());
        this.config.set("peopleNotifySince", this.peopleSince.toISOString());
        this.config.set("walletNotifySince", this.walletSince.toISOString());
        this.config.set("marketplaceNotifySince", this.marketplaceSince.toISOString());

        if (this.storiesPollTimer) {
            clearInterval(this.storiesPollTimer);
        }
        if (this.peoplePollTimer) {
            clearInterval(this.peoplePollTimer);
        }
        if (this.walletPollTimer) {
            clearInterval(this.walletPollTimer);
        }
        if (this.marketplacePollTimer) {
            clearInterval(this.marketplacePollTimer);
        }
    },
    getOnlineUsers: function () {
        return this.onlineUsers;
    },
    getCurrentStories: function () {
        return this.currentStories;
    },
    _showNotification: function () {
        var _this = this;

        if (osType === 'Darwin') {
            this.pendingNotifications[0].show(function () {
                // For OSX
                // Don't attempt to show the next notification
                // until the current is clicked or times out
                // as the OSX Notifier stuff will dismiss the
                // previous notification immediately when a
                // new one is submitted
                _this.pendingNotifications.shift();
                if (_this.pendingNotifications.length > 0) {
                    _this._showNotification();
                }
            });
        } else {
            // For Windows
            // All notifications are sent immediately as they are queued
            // by windows in Tray Notifications and can be bulk seen and 
            // dismissed
            _this.pendingNotifications.shift().show();
        }
    },
    _addNotification: function (notification) {
        this.pendingNotifications.push(notification);
        if (this.pendingNotifications.length === 1) {
            this._showNotification();
        }
    },
    _pollToDisableHighlight: function (notifyType, error, data) {
        if (error || !data.body) {
            console.log("Error: unable to get " + url);
            return false;
        }
        var content = JSON.parse(data.body);
        if (!content || content.status != 'success') {
            console.log("Error: unable to get " + url);
            return false;
        }
        if (!content.total_entries) {
            this.menuNotificationCallback(notifyType, false);
        }
    },
    _pollCommon: function (notifyType, url, since, finished) {

        var _this = this;
        IsInterfaceRunning(function (running) {
            if (running) {
                finished(false);
                return;
            }
            var acctInfo = new AccountInfo();
            var token = acctInfo.accessToken(METAVERSE_SERVER_URL);
            if (!token) {
                return;
            }
            request.get({
                uri: url,
                'auth': {
                    'bearer': token
                }
                }, function (error, data) {

                var maxNotificationItemCount = since.getTime() ? MAX_NOTIFICATION_ITEMS : STARTUP_MAX_NOTIFICATION_ITEMS;
                if (error || !data.body) {
                    console.log("Error: unable to get " + url);
                    finished(false);
                    return;
                }
                var content = JSON.parse(data.body);
                if (!content || content.status != 'success') {
                    console.log("Error: unable to get " + url);
                    finished(false);
                    return;
                }
                if (!content.total_entries) {
                    finished(true, token);
                    return;
                }
                _this.menuNotificationCallback(notifyType, true);
                if (content.total_entries >= maxNotificationItemCount) {
                    _this._addNotification(new HifiNotification(notifyType, content.total_entries));
                } else {
                    var notifyData = []
                    switch (notifyType) {
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
                    notifyData.forEach(function (notifyDataEntry) {
                        _this._addNotification(new HifiNotification(notifyType, notifyDataEntry));
                    });
                }
                finished(true, token);
            });
        });
    },
    pollForStories: function (since) {
        var _this = this;
        var actions = 'announcement';
        var options = [
            'since=' + since.getTime() / 1000,
            'include_actions=announcement',
            'restriction=open,hifi',
            'require_online=true',
            'per_page='+MAX_NOTIFICATION_ITEMS
        ];
        console.log("Polling for stories");
        var url = METAVERSE_SERVER_URL + STORIES_URL + '?' + options.join('&');
        console.log(url);

        _this._pollCommon(NotificationType.GOTO,
            url, 
            since,
            function (success, token) {
                if (success) {
                    var options = [
                        'now=' + new Date().toISOString(),
                        'include_actions=announcement',
                        'restriction=open,hifi',
                        'require_online=true',
                        'per_page=' + MAX_NOTIFICATION_ITEMS
                    ];            
                    var url = METAVERSE_SERVER_URL + STORIES_URL + '?' + options.join('&');
                    // call a second time to determine if there are no more stories and we should
                    // put out the light.
                    request.get({
                        uri: url,
                        'auth': {
                            'bearer': token
                          }
                        }, function (error, data) {
                            if (error || !data.body) {
                                console.log("Error: unable to get " + url);
                                finished(false);
                                return;
                            }
                            var content = JSON.parse(data.body);
                            if (!content || content.status != 'success') {
                                console.log("Error: unable to get " + url);
                                finished(false);
                                return;
                            }

                            if (!content.total_entries) {
                                finished(true, token);
                                return;
                            }
                            if (!content.total_entries) {
                                _this.menuNotificationCallback(NotificationType.GOTO, false);
                            }
                            _this.currentStories = {};
                            content.user_stories.forEach(function (story) {
                                // only show a single instance of each story location
                                // in the menu.  This may cause issues with domains
                                // where the story locations are significantly different
                                // for each story.
                                _this.currentStories[story.place_name] = story;
                            });
                            _this.menuNotificationCallback(NotificationType.GOTO);
                    });
                }
        });
    },
    pollForConnections: function (since) {
        var _this = this;
        var _since = since;
        IsInterfaceRunning(function (running) {
            if (running) {
                return;
            }
            var options = [
                'filter=connections',
                'status=online',
                'page=1',
                'per_page=' + MAX_NOTIFICATION_ITEMS
            ];
            console.log("Polling for connections");
            var url = METAVERSE_SERVER_URL + USERS_URL + '?' + options.join('&');
            console.log(url);
            var acctInfo = new AccountInfo();
            var token = acctInfo.accessToken(METAVERSE_SERVER_URL);
            if (!token) {
                return;
            }
            request.get({
                uri: url,
                'auth': {
                    'bearer': token
                }
            }, function (error, data) {
                // Users is a special case as we keep track of online users locally.
                var maxNotificationItemCount = _since.getTime() ? MAX_NOTIFICATION_ITEMS : STARTUP_MAX_NOTIFICATION_ITEMS;
                if (error || !data.body) {
                    console.log("Error: unable to get " + url);
                    return false;
                }
                var content = JSON.parse(data.body);
                if (!content || content.status != 'success') {
                    console.log("Error: unable to get " + url);
                    return false;
                }
                if (!content.total_entries) {
                    _this.menuNotificationCallback(NotificationType.PEOPLE, false);
                    _this.onlineUsers = {};
                    return;
                }

                var currentUsers = {};
                var newUsers = new Set([]);
                content.data.users.forEach(function (user) {
                    currentUsers[user.username] = user;
                    if (!(user.username in _this.onlineUsers)) {
                        newUsers.add(user);
                        _this.onlineUsers[user.username] = user;     
                    }
                });
                _this.onlineUsers = currentUsers;
                if (newUsers.size) {
                    _this.menuNotificationCallback(NotificationType.PEOPLE, true);
                }

                if (newUsers.size >= maxNotificationItemCount) {
                    _this._addNotification(new HifiNotification(NotificationType.PEOPLE, newUsers.size));
                    return;
                }
                newUsers.forEach(function (user) {
                    _this._addNotification(new HifiNotification(NotificationType.PEOPLE, user));
                });
            });
        });
    },
    pollForEconomicActivity: function (since) {
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
        _this._pollCommon(NotificationType.WALLET, url, since, function () {});
    },
    pollForMarketplaceUpdates: function (since) {
        var _this = this;
        var options = [
            'since=' + since.getTime() / 1000,
            'page=1',
            'per_page=' + MAX_NOTIFICATION_ITEMS
        ];
        console.log("Polling for marketplace update");
        var url = METAVERSE_SERVER_URL + UPDATES_URL + '?' + options.join('&');
        console.log(url);
        _this._pollCommon(NotificationType.MARKETPLACE, url, since, function (success, token) {
            if (success) {
                var options = [
                    'page=1',
                    'per_page=1'
                ];
                var url = METAVERSE_SERVER_URL + UPDATES_URL + '?' + options.join('&');
                request.get({
                        uri: url,
                        'auth': {
                            'bearer': token
                        }
                    }, function (error, data) {
                        _this._pollToDisableHighlight(NotificationType.MARKETPLACE, error, data);
                });
            }
        });
    }
};

exports.HifiNotifications = HifiNotifications;
exports.NotificationType = NotificationType;