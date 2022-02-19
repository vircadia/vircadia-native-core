"use strict";
//
//  places.js
//
//  Created by Alezia Kurdis, January 1st, 2022.
//  Copyright 2022 Overte e.V.
//
//  Generate an explore app based on the differents source of placename data.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
(function() {
    var jsMainFileName = "places.js";
    var ROOT = Script.resolvePath('').split(jsMainFileName)[0];
       
    var metaverseServers = [];
    var SETTING_METAVERSE_TO_FETCH = "placesAppMetaverseToFetch";
    var SETTING_PINNED_METAVERSE = "placesAppPinnedMetaverse";
         
    var httpRequest = null;
    var placesData;
    var portalList = [];

    var nbrPlacesNoProtocolMatch = 0;
    var nbrPlaceProtocolKnown = 0;
    
    var APP_NAME = "PLACES";
    var APP_URL = ROOT + "places.html";
    var APP_ICON_INACTIVE = ROOT + "icons/appicon_i.png";
    var APP_ICON_ACTIVE = ROOT + "icons/appicon_a.png";
    var appStatus = false;
    var channel = "com.overte.places";    

    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

    tablet.screenChanged.connect(onScreenChanged);

    var button = tablet.addButton({
        text: APP_NAME,
        icon: APP_ICON_INACTIVE,
        activeIcon: APP_ICON_ACTIVE,
        sortOrder: 8
    });
    
    var timestamp = 0;
    var INTERCALL_DELAY = 200; //0.3 sec
    var PERSISTENCE_ORDERING_CYCLE = 5 * 24 * 3600 * 1000; //5 days
    
    function clicked(){
        if (appStatus === true) {
            tablet.webEventReceived.disconnect(onAppWebEventReceived);
            tablet.gotoHomeScreen();
            appStatus = false;
        } else {
            tablet.gotoWebScreen(APP_URL);
            tablet.webEventReceived.connect(onAppWebEventReceived);
            appStatus = true;
        }

        button.editProperties({
            isActive: appStatus
        });
    }

    button.clicked.connect(clicked);


    function onAppWebEventReceived(message) {
        var d = new Date();
        var n = d.getTime();
        
        var messageObj = JSON.parse(message);
        if (messageObj.channel === channel) {
            if (messageObj.action === "READY_FOR_CONTENT" && (n - timestamp) > INTERCALL_DELAY) {
                d = new Date();
                timestamp = d.getTime();
                transmitPortalList();

                sendCurrentLocationToUI();
                
            } else if (messageObj.action === "TELEPORT" && (n - timestamp) > INTERCALL_DELAY) {
                d = new Date();
                timestamp = d.getTime();

                if (messageObj.address.length > 0) {
                    Window.location = messageObj.address;
                }                
                
            } else if (messageObj.action === "GO_HOME" && (n - timestamp) > INTERCALL_DELAY) {
                if (LocationBookmarks.getHomeLocationAddress()) {
                    location.handleLookupString(LocationBookmarks.getHomeLocationAddress());
                } else {
                    location.goToLocalSandbox();
                }                
            } else if (messageObj.action === "GO_BACK" && (n - timestamp) > INTERCALL_DELAY) {
                location.goBack();
            } else if (messageObj.action === "GO_FORWARD" && (n - timestamp) > INTERCALL_DELAY) {
                location.goForward();
            } else if (messageObj.action === "PIN_META" && (n - timestamp) > INTERCALL_DELAY) {
                d = new Date();
                timestamp = d.getTime();
                metaverseServers[messageObj.metaverseIndex].pinned = messageObj.value;
                savePinnedMetaverseSetting();
            } else if (messageObj.action === "FETCH_META" && (n - timestamp) > INTERCALL_DELAY) {
                d = new Date();
                timestamp = d.getTime();
                metaverseServers[messageObj.metaverseIndex].fetch = messageObj.value;
                saveMetaverseToFetchSetting();                
                
            }
        }
    }

    function savePinnedMetaverseSetting() {
        var pinnedServers = [];
        for (var q = 0; q < metaverseServers.length; q++) {
            if (metaverseServers[q].pinned) {
                pinnedServers.push(metaverseServers[q].url);
            }
        }
        Settings.setValue(SETTING_PINNED_METAVERSE, pinnedServers);
    }

    function saveMetaverseToFetchSetting() {
        var fetchedServers = [];
        for (var q = 0; q < metaverseServers.length; q++) {
            if (metaverseServers[q].fetch) {
                fetchedServers.push(metaverseServers[q].url);
            }
        }
        Settings.setValue(SETTING_METAVERSE_TO_FETCH, fetchedServers);        
    }

    function onHostChanged(host) {
        sendCurrentLocationToUI();
    }

    location.hostChanged.connect(onHostChanged);
    
    function sendCurrentLocationToUI() {
        var currentLocationMessage = {
            "channel": channel,
            "action": "CURRENT_LOCATION",
            "data": location.href
        };

        tablet.emitScriptEvent(currentLocationMessage);        
    }
    
    function onScreenChanged(type, url) {
        if (type == "Web" && url.indexOf(APP_URL) != -1) {
            appStatus = true;
        } else {
            appStatus = false;
        }
        
        button.editProperties({
            isActive: appStatus
        });
    }

    function transmitPortalList() {
        metaverseServers = [];
        buildMetaverseServerList();
        
        portalList = [];
        nbrPlacesNoProtocolMatch = 0;
        nbrPlaceProtocolKnown = 0;
        var extractedData;
        
        for (var i = 0; i < metaverseServers.length; i++ ) {
            if (metaverseServers[i].fetch === true) {
                extractedData = getContent(metaverseServers[i].url + "/api/v1/places?status=online" + "&acash=" + Math.floor(Math.random() * 999999));
                try {
                    placesData = JSON.parse(extractedData);
                    processData(metaverseServers[i]);
                } catch(e) {
                    placesData = {};
                }
                httpRequest = null;
            }
        }

        //################### TO REMOVED ONCE NO MORE USED #####################
        getDeprecatedBeaconsData();
        //################### END: TO REMOVED ONCE NO MORE USED #####################
        
        addUtilityPortals();
        
        portalList.sort(sortOrder);
        
        var percentProtocolRejected = Math.floor((nbrPlacesNoProtocolMatch/nbrPlaceProtocolKnown) * 100);
        
        var warning = "";
        if (percentProtocolRejected > 50) {
            warning = "WARNING: " + percentProtocolRejected + "% of the places are not listed because they are running under a different protocol. Maybe consider to upgrade.";
        }

        var message = {
            "channel": channel,
            "action": "PLACE_DATA",
            "data": portalList,
            "warning": warning,
            "metaverseServers": metaverseServers
        };

        tablet.emitScriptEvent(message);
        
    };

    function getFederationData() {
        /*
        //If federation.json is got from the Metaverse Server (not implemented yet)
        var fedDirectoryUrl = AccountServices.metaverseServerURL + "/federation.json";
        var extractedFedData = getContent(fedDirectoryUrl);
        */

        /*
        //If federation.json is got from a web storage
        var fedDirectoryUrl = ROOT + "federation.json"; + "?version=" + Math.floor(Math.random() * 999999);
        var extractedFedData = getContent(fedDirectoryUrl);
        */

        //if federation.json is local, on the user installation
        var extractedFedData = JSON.stringify(Script.require("./federation.json"));

        return extractedFedData;

    }

    function buildMetaverseServerList () {
        var extractedFedData = getFederationData();

        var pinnedMetaverses = Settings.getValue(SETTING_PINNED_METAVERSE, []);
        var metaversesToFetch = Settings.getValue(SETTING_METAVERSE_TO_FETCH, []);

        var federation = [];
        try {
            federation = JSON.parse(extractedFedData);
        } catch(e) {
            federation = [];
        }        
        var currentFound = false;
        var region, pinned, fetch, order, metaverse;
        for (var i=0; i < federation.length; i++) {
            if (federation[i].node === AccountServices.metaverseServerURL) {
                region = "local";
                order = "A";
                fetch = true;
                pinned = false;                
                currentFound = true;                
            } else {
                region = "federation";
                order = "F";
                fetch = false;
                pinned = false;
            }
            
            metaverse = {
                "url": federation[i].node,
                "region": region,
                "fetch": fetch,
                "pinned": pinned,
                "order": order
            };
            metaverseServers.push(metaverse);
        }
        if (!currentFound) {
            metaverse = {
                "url": AccountServices.metaverseServerURL,
                "region": "local",
                "fetch": true,
                "pinned": false,
                "order": "A"
            };
            metaverseServers.push(metaverse);
        }
 
        for (i = 0; i < pinnedMetaverses.length; i++) {
            var target = pinnedMetaverses[i];
            var found = false;
            for (var k = 0; k < metaverseServers.length; k++) {
                if (metaverseServers[k].url === target) {
                    metaverseServers[k].pinned = true;
                    found = true;
                    break;
                }
            }
            if (!found) {
                metaverse = {
                    "url": target,
                    "region": "external",
                    "fetch": false,
                    "pinned": true,
                    "order": "Z"
                };
                metaverseServers.push(metaverse);                
            }
        }

        for (i = 0; i < metaversesToFetch.length; i++) {
            var target = metaversesToFetch[i];
            for (var k = 0; k < metaverseServers.length; k++) {
                if (metaverseServers[k].url === target) {
                    metaverseServers[k].fetch = true;
                    break;
                }
            }
        }
        
        metaverseServers.sort(sortOrder);
    }

    function getContent(url) {
        httpRequest = new XMLHttpRequest();
        httpRequest.open("GET", url, false); // false for synchronous request
        httpRequest.send( null );
        return httpRequest.responseText;
    }

    function processData(metaverseInfo){
        var supportedProtocole = Window.protocolSignature();

        var places = placesData.data.places;
        for (var i = 0;i < places.length; i++) {

            var region, category, accessStatus;
            
            var description = (places[i].description ? places[i].description : "");
            var thumbnail = (places[i].thumbnail ? places[i].thumbnail : "");

            if ( places[i].domain.protocol_version === supportedProtocole ) {
                  
                    region = metaverseInfo.order;

                    if ( thumbnail.substr(0, 4).toLocaleLowerCase() !== "http") {
                        category = "O"; //Other
                    } else {
                        category = "A"; //Attraction                        
                    }
                    
                    if (places[i].domain.num_users > 0) {
                        if (places[i].domain.num_users >= places[i].domain.capacity && places[i].domain.capacity !== 0) {
                            accessStatus = "FULL";
                        } else {
                            accessStatus = "LIFE";
                        }
                    } else {
                        accessStatus = "NOBODY";
                    }                 

                    var portal = {
                        "order": category + "_" + region + "_" + getSeededRandomForString(places[i].id),
                        "category": category,
                        "accessStatus": accessStatus,
                        "name": places[i].name,
                        "description": description,
                        "thumbnail": thumbnail,
                        "maturity": places[i].maturity,
                        "address": places[i].address,
                        "current_attendance": places[i].domain.num_users,
                        "id": places[i].id,
                        "visibility": places[i].visibility,
                        "capacity": places[i].domain.capacity,
                        "tags": getListFromArray(places[i].tags),
                        "managers": getListFromArray(places[i].managers),
                        "domain": places[i].domain.name,
                        "domainOrder": aplphabetize(zeroPad(places[i].domain.num_users, 6)) + "_" + places[i].domain.name + "_" + places[i].name,
                        "metaverseServer": metaverseInfo.url,
                        "metaverseRegion": metaverseInfo.region
                    };
                    portalList.push(portal);

            } else {
                nbrPlacesNoProtocolMatch++;
            }
        }
        
        nbrPlaceProtocolKnown = nbrPlaceProtocolKnown + places.length;
    
    }

    //################### CODE TO REMOVED ONCE NO MORE USED #####################
    function getDeprecatedBeaconsData() {
        var url = "https://metaverse.vircadia.com/interim/d-goto/app/goto.json";
        httpRequest = new XMLHttpRequest();
        httpRequest.open("GET", url, false); // false for synchronous request
        httpRequest.send( null );
        var extractedData = httpRequest.responseText;
        
        httpRequest = null;
        
        var places;
        try {
            places = JSON.parse(extractedData);
        } catch(e) {
            places = {};
        }

        for (var i = 0;i < places.length; i++) {

            var category, accessStatus;
            
            var description = "...";
            var thumbnail = "";
            category = "U"; //uncertain

            if (places[i].People > 0) {
                accessStatus = "LIFE";
            } else {
                accessStatus = "NOBODY";
            }                 
            
            var shortenName = places[i]["Domain Name"].substr(0, 24);
            

            var portal = {
                "order": category + "_Z_" + getSeededRandomForString(places[i]["Domain Name"]),
                "category": category,
                "accessStatus": accessStatus,
                "name": shortenName,
                "description": description,
                "thumbnail": thumbnail,
                "maturity": "unrated",
                "address": places[i].Visit,
                "current_attendance": places[i].People,
                "id": "BEACON" + i,
                "visibility": "open",
                "capacity": 0,
                "tags": "",
                "managers": places[i].Owner,
                "domain": "UNKNOWN (Beacon)",
                "domainOrder": "ZZZZZZZZZZZZZUA",
                "metaverseServer": "",
                "metaverseRegion": "external"                
            };
            portalList.push(portal);
        }
    }
    //################### END::: CODE TO REMOVED ONCE NO MORE USED #####################

    function addUtilityPortals() {
        var localHostPortal = {
            "order": "Z_Z_AAAAAA",
            "category": "Z",
            "accessStatus": "NOBODY",
            "name": "localhost",
            "description": "",
            "thumbnail": "",
            "maturity": "unrated",
            "address": "localhost",
            "current_attendance": 0,
            "id": "",
            "visibility": "open",
            "capacity": 0,
            "tags": "",
            "managers": "",
            "domain": "",
            "domainOrder": "ZZZZZZZZZZZZZZA",
            "metaverseServer": "",
            "metaverseRegion": "local"
        };
        portalList.push(localHostPortal);

        var tutorialPortal = {
            "order": "Z_Z_AAAAAZ",
            "category": "Z",
            "accessStatus": "NOBODY",
            "name": "tutorial",
            "description": "",
            "thumbnail": "",
            "maturity": "unrated",
            "address": "file:///~/serverless/tutorial.json",
            "current_attendance": 0,
            "id": "",
            "visibility": "open",
            "capacity": 0,
            "tags": "",
            "managers": "",
            "domain": "",
            "domainOrder": "ZZZZZZZZZZZZZZZ",
            "metaverseServer": "",
            "metaverseRegion": "local"            
        };
        portalList.push(tutorialPortal);
        
    }

    function aplphabetize(num) {
        var numbstring = num.toString();
        var newChar = "JIHGFEDCBA";
        var refChar = "0123456789";
        var processed = "";
        for (var j=0; j < numbstring.length; j++) {
            processed = processed + newChar.substr(refChar.indexOf(numbstring.charAt(j)),1);
        }
        return processed;
    }

    function getListFromArray(dataArray) {
        var dataList = "";
        if (dataArray !== undefined && dataArray.length > 0) {
            for (var k = 0; k < dataArray.length; k++) {
                if (k !== 0) {
                    dataList += ", "; 
                }
                dataList += dataArray[k];
            }
            if (dataArray.length > 1){
                dataList += ".";
            }
        }
        
        return dataList;
    }

    function sortOrder(a, b) {
        var orderA = a.order.toUpperCase();
        var orderB = b.order.toUpperCase();
        if (orderA > orderB) {
            return 1;    
        } else if (orderA < orderB) {
            return -1;
        }
        if (a.order > b.order) {
            return 1;    
        } else if (a.order < b.order) {
            return -1;
        }
        return 0;
    }

    function zeroPad(num, places) {
        var zero = places - num.toString().length + 1;
        return Array(+(zero > 0 && zero)).join("0") + num;
    }

    function getFrequentPlaces(list) {
        var count = {};
        list.forEach(function(list) {
            count[list] = (count[list] || 0) + 1;
        });
        return count;
    }

    //####### seed random library ################
    Math.seed = 75;

    Math.seededRandom = function(max, min) {
        max = max || 1;
        min = min || 0;
        Math.seed = (Math.seed * 9301 + 49297) % 233280;
        var rnd = Math.seed / 233280;
        return min + rnd * (max - min);
    }

    function getStringScore(str) {
        var score = 0;
        for (var j = 0; j < str.length; j++){
            score += str.charAt(j).charCodeAt(0) + 1;
        }
        return score;
    }

    function getSeededRandomForString(str) {
        var score = getStringScore(str);
        var d = new Date();
        var n = d.getTime();
        var currentSeed = Math.floor(n / PERSISTENCE_ORDERING_CYCLE);
        Math.seed = score * currentSeed;
        return zeroPad(Math.floor(Math.seededRandom() * 100000),5);
    }
    //####### END of seed random library ################

    function cleanup() {

        if (appStatus) {
            tablet.gotoHomeScreen();
            tablet.webEventReceived.disconnect(onAppWebEventReceived);
        }

        tablet.screenChanged.disconnect(onScreenChanged);
        tablet.removeButton(button);
    }

    Script.scriptEnding.connect(cleanup);
}());
