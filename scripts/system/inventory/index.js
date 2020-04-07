//
//  index.js
//
//  Created by kasenvr@gmail.com on 4 Apr 2020
//  Copyright 2020 Vircadia Contributors
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var vue_this;

function browserDevelopment() {
    if (typeof EventBridge !== 'undefined') {
        return false; // We are in the browser, probably for development purposes.
    } else {
        return true; // We are in Vircadia.
    }
}

if (!browserDevelopment()) {
    
    EventBridge.scriptEventReceived.connect(function(receivedCommand) {
        receivedCommand = JSON.parse(receivedCommand);
        // alert("RECEIVED COMMAND:" + receivedCommand.command)
        if (receivedCommand.app == "inventory") {
        // We route the data based on the command given.
            if (receivedCommand.command == 'script-to-web-inventory') {
                // alert("INVENTORY RECEIVED ON APP:" + JSON.stringify(receivedCommand.data));
                vue_this.receiveInventory(receivedCommand.data);
            }
    
            if (receivedCommand.command == 'script-to-web-receiving-item') {
                // alert("RECEIVING ITEM OFFER:" + JSON.stringify(receivedCommand.data));
                vue_this.receivingItem(receivedCommand.data);
            }
    
            if (receivedCommand.command == 'script-to-web-nearby-users') {
                // alert("RECEIVING NEARBY USERS:" + JSON.stringify(receivedCommand.data));
                vue_this.receiveNearbyUsers(receivedCommand.data);
            }
            
            if (receivedCommand.command == 'script-to-web-settings') {
                // alert("RECEIVING SETTINGS:" + JSON.stringify(receivedCommand.data));
                vue_this.receiveSettings(receivedCommand.data);
            }
    
        }
    });
    
}

new Vue({
    el: '#inventoryApp',
    vuetify: new Vuetify(),
    data: () => ({
        items: [
            {
                "type": "script",
                "name": "VRGrabScale",
                "url": "https://gooawefaweawfgle.com/vr.js",
                "uuid": "54254354353",
            },
            {
                "folder": true,
                "name": "Test Folder",
                "items": [
                    {
                        "type": "script",
                        "name": "TESTFOLDERSCRIPT",
                        "url": "https://googfdafsgaergale.com/vr.js",
                        "uuid": "54hgfhgf25fdfadf4354353",
                    },
                ],
                "uuid:": "54354363wgsegs45ujs",
            },
            {
                "type": "script",
                "name": "VRGrabScale",
                "url": "https://googfdafsgaergale.com/vr.js",
                "uuid": "54hgfhgf254354353",
            },
            {
                "type": "script",
                "name": "TEST",
                "url": "https://gooadfdagle.com/vr.js",
                "uuid": "542rfwat4t54354353",
            },
            {
                "type": "script",
                "name": "TESTLONGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG",
                "url": "https://googfdaffle.com/vrLONGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG.js",
                "uuid": "5425ggsrg45354353",
            },
            {
                "type": "avatar",
                "name": "AVI",
                "url": "https://googlfadfe.com/vr.fst",
                "uuid": "542gregg45s3g4354353",
            },
            {
                "type": "avatar",
                "name": "AVI",
                "url": "https://googlefdaf.com/vr.fst",
                "uuid": "5420798-087-54354353",
            },
            {
                "type": "model",
                "name": "3D MODEL",
                "url": "https://googlee.com/vr.fbx",
                "uuid": "54254354980-7667jt353",
            },
            {
                "type": "model",
                "name": "3D MODEL",
                "url": "https://googleee.com/vr.fbx",
                "uuid": "542543sg45s4gg54353",
            },
        ],
        iconType: {
            "script": {
                "icon": "mdi-code-tags",
                "color": "red",
            },
            "model": {
                "icon": "mdi-video-3d",
                "color": "green",
            },
            "avatar": {
                "icon": "mdi-account-convert",
                "color": "purple",
            },
            "unknown": {
                "icon": "mdi-help",
                "color": "grey",
            }
        },
        // The URL is the key (to finding the item we want) so we want to keep track of that.
        removeDialog: {
            show: false,
            uuid: null,
        },
        createFolderDialog: {
            show: false,
            valid: false,
            data: {
                "name": null,
            },
        },
        addDialog: {
            show: false,
            valid: false,
            data: {
                "name": null,
                "url": null,
            },
        },
        editDialog: {
            show: false,
            valid: false,
            uuid: null, //
            data: {
                "type": null,
                "name": null,
                "url": null,
            },
        },
        receiveDialog: {
            show: false,
            valid: false,
            data: {
                "user": null,
                "name": null,
                "type": null,
                "url": null,
            },
        },
        shareDialog: {
            show: false,
            valid: false,
            data: {
                "uuid": null, // UUID of the item you want to share. THIS IS THE KEY.
                "url": null, // The item you want to share.
                "recipient": null,
            }
        },
        nearbyUsers: [
            {
                name: "Who",
                uuid: "{4131531653652562}",
            },
            {
                name: "Is",
                uuid: "{4131531653756756576543652562}",
            },
            {
                name: "This?",
                uuid: "{4131531676575653652562}",
            },
        ],
        sortBy: "alphabetical",
        settings: {
            displayDensity: {
                "size": 1,
                "labels": [
                    "List",
                    "Compact",
                    "Large",
                ],
            },
        },
        darkTheme: true,
        drawer: false,
    }),
    created: function () {
        vue_this = this;
        this.$vuetify.theme.dark = this.darkTheme;
        
        this.sendAppMessage("ready", "");
    },
    methods: {
        createUUID: function() {
            // http://www.ietf.org/rfc/rfc4122.txt
            var s = [];
            var hexDigits = "0123456789abcdef";
            for (var i = 0; i < 36; i++) {
                s[i] = hexDigits.substr(Math.floor(Math.random() * 0x10), 1);
            }
            s[14] = "4";  // bits 12-15 of the time_hi_and_version field to 0010
            s[19] = hexDigits.substr((s[19] & 0x3) | 0x8, 1);  // bits 6-7 of the clock_seq_hi_and_reserved to 01
            s[8] = s[13] = s[18] = s[23] = "-";

            var uuid = s.join("");
            return uuid;
        },
        pushToItems: function(type, name, url) {
            var itemToPush =             
            {
                "type": type,
                "name": name,
                "url": url,
                "uuid": this.createUUID(),
            };
            
            this.items.push(itemToPush);
        },
        pushFolderToItems: function(name) {
            var folderToPush =             
            {
                "folder": true,
                "name": name,
                "items": [],
                "uuid": this.createUUID(),
            };
            
            this.items.push(folderToPush);
        },
        checkFileType: function(fileType) {
            var detectedItemType = null;
            
            switch (fileType) {
                // Model Cases
                case ".fbx":
                    detectedItemType = "model";
                    break;
                case ".gltf":
                    detectedItemType = "model";
                    break;
                // Script Cases
                case ".js":
                    detectedItemType = "script";
                    break;
                // Avatar Cases
                case ".fst":
                    detectedItemType = "avatar";
                    break;
            }
            
            if (detectedItemType == null) {
                // This is not a known item...
                detectedItemType = "unknown";
            }
            
            return detectedItemType;
        },
        checkItemType: function(itemType) {
            var detectedItemType = null;
            
            switch (itemType) {
                case "model":
                    detectedItemType = "model";
                    break;
                case "avatar":
                    detectedItemType = "avatar";
                    break;
                case "script":
                    detectedItemType = "script";
                    break;
            }
            
            if (detectedItemType == null) {
                // This is not a known item type...
                detectedItemType = "unknown";
            }
            
            return detectedItemType;
        },
        createFolder: function(name) {
            this.pushFolderToItems(name);
        },
        addItem: function(name, url) {
            var extensionRegex = /\.[0-9a-z]+$/i; // to detect the file type based on extension in the URL.
            var detectedFileType = url.match(extensionRegex);
            var itemType;
                        
            if (detectedFileType == null || detectedFileType[0] == null) {
                itemType = "unknown";
            } else {
                itemType = this.checkFileType(detectedFileType[0]);
            }
            
            this.pushToItems(itemType, name, url);
            
            this.addDialog.data.name = null;
            this.addDialog.data.url = null;
        },
        removeItem: function(uuid) {
            for (i = 0; i < this.items.length; i++) {
                if (this.items[i].uuid == uuid) {
                    this.items.splice(i, 1);
                }
            }
        },
        editItem: function(uuid) {
            for (i = 0; i < this.items.length; i++) {
                if (this.items[i].uuid == uuid) {
                    this.items[i].type = this.checkItemType(this.editDialog.data.type);
                    this.items[i].name = this.editDialog.data.name;
                    this.items[i].url = this.editDialog.data.url;
                }
            }
        },
        receivingItem: function(data) {
            if (this.receiveDialog.show != true) { // Do not accept offers if the user is already receiving an offer.
                this.receiveDialog.data.user = data.data.user;
                this.receiveDialog.data.type = data.data.type;
                this.receiveDialog.data.name = data.data.name;
                this.receiveDialog.data.url = data.data.url;
                
                this.receiveDialog.show = true;
            }
            
        },
        shareItem: function(uuid, url) {
            var typeToShare;
            var nameToShare;
            
            for (i = 0; i < this.items.length; i++) {
                if (this.items[i].uuid == uuid) {
                    typeToShare = this.items[i].type;
                    nameToShare = this.items[i].name;
                }
            }
            
            // alert("type" + typeToShare + "name" + nameToShare);
            this.sendAppMessage("share-item", {
                "type": typeToShare,
                "name": nameToShare,
                "url": this.shareDialog.data.url,
                "recipient": this.shareDialog.data.recipient,
            });
        },
        acceptItem: function() {
            this.pushToItems(this.checkItemType(this.receiveDialog.data.type), this.receiveDialog.data.name, this.receiveDialog.data.url);
        },
        useItem: function(type, url) {
            this.sendAppMessage("use-item", { 
                "type": type, 
                "url": url 
            });
        },
        sendInventory: function() {
            this.sendAppMessage("web-to-script-inventory", this.items );
        },
        receiveInventory: function(receivedInventory) {
            if (!receivedInventory) {
                this.items = [];
            } else {
                this.items = receivedInventory;
            }
        },
        sendSettings: function() {
            this.sendAppMessage("web-to-script-settings", this.settings );
        },
        receiveSettings: function(receivedSettings) {
            if (!receivedSettings) {
                // Don't do anything, let the defaults stand. Otherwise, it will break the app.
            } else {
                this.settings = receivedSettings;
            }
        },
        displayIcon: function(itemType) {
            return this.iconType[itemType].icon;
        },
        getIconColor: function(itemType) {
            return this.iconType[itemType].color;
        },
        receiveNearbyUsers: function(receivedUsers) {
            if (!receivedUsers) {
                this.nearbyUsers = [];
            } else {
                this.nearbyUsers = receivedUsers;
            }
        },
        sendAppMessage: function(command, data) {
            var JSONtoSend = {
                "app": "inventory",
                "command": command,
                "data": data
            };
                        
            if (!browserDevelopment()) {
                EventBridge.emitWebEvent(JSON.stringify(JSONtoSend));
            } else {
                alert(JSON.stringify(JSONtoSend));
            }
        },
    },
    watch: {
        // Whenever the item list changes, this will notice and then send it to the script to be saved.
        items: {
            deep: true,
            handler() {
                this.sendInventory();
            }
        }, // Whenever the settings change, we want to save that state.
        settings: {
            deep: true,
            handler() {
                this.sendSettings();
            }
        }
    },
    computed: {

    }
})