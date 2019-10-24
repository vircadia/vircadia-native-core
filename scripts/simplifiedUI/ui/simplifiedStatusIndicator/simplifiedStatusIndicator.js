//
//  simplifiedStatusIndicator.js
//
//  Created by Robin Wilson on 2019-05-17
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


function SimplifiedStatusIndicator() {
    var that = this;
    var DEBUG = false;

    // #region HEARTBEAT

    // Send heartbeat with updates to database
    // When this stops, the database will set status to offline
    var HEARTBEAT_TIMEOUT_MS = 5000,
        heartbeat;
    function startHeartbeatTimer() {
        if (heartbeat) {
            Script.clearTimeout(heartbeat);
            heartbeat = false;
        }

        heartbeat = Script.setTimeout(function() {
            heartbeat = false;
            getStatus(setStatus);
        }, HEARTBEAT_TIMEOUT_MS);
    }

    // #endregion HEARTBEAT


    // #region SEND/GET STATUS REQUEST

    function setStatusExternally(newStatus) {
        if (!newStatus) {
            return;
        }

        setStatus(newStatus);
    }


    var request = Script.require('request').request,
        REQUEST_URL = "https://highfidelity.co/api/statusIndicator/";
    function setStatus(newStatus) {
        if (heartbeat) {
            Script.clearTimeout(heartbeat);
            heartbeat = false;
        }

        if (newStatus && currentStatus !== newStatus) {
            currentStatus = newStatus;
            that.statusChanged();
        }

        var queryParamString = "type=heartbeat";
        queryParamString += "&username=" + AccountServices.username;

        var displayNameToSend = MyAvatar.displayName;
        
        queryParamString += "&displayName=" + displayNameToSend;
        queryParamString += "&status=" + currentStatus;
        var domainID = location.domainID;
        domainID = domainID.substring(1, domainID.length - 1);
        queryParamString += "&organization=" + domainID;

        var uri = REQUEST_URL + "?" + queryParamString;

        if (DEBUG) {
            console.log("simplifiedStatusIndicator: setStatus: " + uri);
        }

        request({
            uri: uri
        }, function (error, response) {
            startHeartbeatTimer();

            if (error || !response || response.status !== "success") {
                console.error("Error with setStatus: " + JSON.stringify(response));
                return;
            }
        });
    }


    // Get status from database
    function getStatus(callback) {
        var queryParamString = "type=getStatus";
        queryParamString += "&username=" + AccountServices.username;

        var uri = REQUEST_URL + "?" + queryParamString;

        if (DEBUG) {
            console.log("simplifiedStatusIndicator: getStatus: " + uri);
        }

        request({
            uri: uri
        }, function (error, response) {
            if (error || !response || response.status !== "success") {
                console.error("Error with getStatus: " + JSON.stringify(response));
            } else if (response.data.userStatus.toLowerCase() !== "offline") {
                if (response.data.userStatus !== currentStatus) {
                    currentStatus = response.data.userStatus;
                    that.statusChanged();
                }
            }
            
            if (callback) {
                callback();
            }
        });
    }


    function getLocalStatus() {
        return currentStatus;
    }

    // #endregion SEND/GET STATUS REQUEST


    // #region SIGNALS

    function updateProperties(properties) {
        // Overwrite with the given properties
        var overwriteableKeys = ["statusChanged"];
        Object.keys(properties).forEach(function (key) {
            if (overwriteableKeys.indexOf(key) > -1) {
                that[key] = properties[key];
            }
        });
    }

    
    var currentStatus = "available"; // Default is available
    function toggleStatus() {
        if (currentStatus === "busy") {
            currentStatus = "available";
        // Else if current status is "available" OR anything else (custom)
        } else {
            currentStatus = "busy";
        }

        that.statusChanged();

        setStatus();
    }


    // When avatar becomes active from being away
    // Set status back to previousStatus
    function onWentActive() {
        if (currentStatus !== previousStatus) {
            currentStatus = previousStatus;
            that.statusChanged();
        }
        setStatus();
    }


    // When avatar goes away, set status to busy
    var previousStatus = "available";
    function onWentAway() {
        previousStatus = currentStatus;
        if (currentStatus !== "busy") {
            currentStatus = "busy";
            that.statusChanged();
        }
        setStatus();
    }


    // Domain changed update avatar location
    function onDomainChanged() {
        var queryParamString = "type=updateEmployee";
        queryParamString += "&username=" + AccountServices.username;
        queryParamString += "&location=unknown";

        var uri = REQUEST_URL + "?" + queryParamString;

        if (DEBUG) {
            console.log("simplifiedStatusIndicator: onDomainChanged: " + uri);
        }

        request({
            uri: uri
        }, function (error, response) {
            if (error || !response || response.status !== "success") {
                console.error("Error with onDomainChanged: " + JSON.stringify(response));
            } else {
                // successfully sent updateLocation
                if (DEBUG) {
                    console.log("simplifiedStatusIndicator: Successfully updated location after domain change.");
                }
            }
        });
    }


    function statusChanged() {

    }

    // #endregion SIGNALS


    // #region APP LIFETIME

    // Creates the app button and sets up signals and hearbeat
    function startup() {
        MyAvatar.wentAway.connect(onWentAway);
        MyAvatar.wentActive.connect(onWentActive);
        MyAvatar.displayNameChanged.connect(setStatus);
        Window.domainChanged.connect(onDomainChanged);

        getStatus(setStatus);

        Script.scriptEnding.connect(unload);
    }


    // Cleans up timeouts, signals, and overlays
    function unload() {
        MyAvatar.wentAway.disconnect(onWentAway);
        MyAvatar.wentActive.disconnect(onWentActive);
        MyAvatar.displayNameChanged.disconnect(setStatus);
        Window.domainChanged.disconnect(onDomainChanged);
        if (heartbeat) {
            Script.clearTimeout(heartbeat);
            heartbeat = false;
        }
    }

    // #endregion APP LIFETIME

    that.toggleStatus = toggleStatus;
    that.setStatus = setStatus;
    that.getLocalStatus = getLocalStatus;
    that.statusChanged = statusChanged;
    that.updateProperties = updateProperties;

    startup();
}

var simplifiedStatusIndicator = new SimplifiedStatusIndicator();

module.exports = simplifiedStatusIndicator;