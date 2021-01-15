<!--
//
//  App.vue
//
//  Created by Kalila L. - somnilibertas@gmail.com on 7 Apr 2020
//  Copyright 2020 Vircadia and contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
-->

<style>
    @import './assets/styles/styles.css';
</style>

<template>
    <v-app>
        
        <!-- ### MENU / NAVIGATION ### -->

        <v-app-bar
            app
        >

            <v-app-bar-nav-icon
                @click="drawer = !drawer"
            ></v-app-bar-nav-icon>

            <v-toolbar-title>
                {{ settingsStore.currentView }}
            </v-toolbar-title>

            <v-text-field
                label="Search"
                v-model="searchBoxStore"
                class="mx-5 mt-7"
                clearable
                :disabled="settingsStore.currentView === 'Inventory'"
            ></v-text-field>

            <v-spacer></v-spacer>
            
            <v-badge
                bordered
                color="primary"
                :value="receivingItemsDialogStore.data.receivingItemQueue.length"
                :content="receivingItemsDialogStore.data.receivingItemQueue.length"
                overlap
                class="mx-5"
                v-show="settingsStore.currentView === 'Inventory'"
            >
                <v-btn
                    small 
                    color="red" 
                    fab 
                    @click="receivingItemsDialogStore.show = true; sendAppMessage('web-to-script-request-receiving-item-queue', '')"
                >
                    <v-icon>
                        mdi-tray-full
                    </v-icon>
                </v-btn>
            </v-badge>
            
            <v-menu 
                bottom 
                left 
            >
                <template v-slot:activator="{ on }">
                    <v-btn 
                        large
                        color="primary"
                        v-on="on"
                        v-show="settingsStore.currentView === 'Inventory'"
                    >
                        <h4>Sort</h4>
                    </v-btn>
                </template>

                <v-list color="grey darken-3">
                    <v-list-item
                        @click="sortTopInventory('az')"
                    >
                        <v-list-item-title>A-Z</v-list-item-title>
                        <v-list-item-action>
                            <v-icon large>mdi-sort-alphabetical-ascending</v-icon>
                        </v-list-item-action>
                    </v-list-item>
                    <v-list-item
                        @click="sortTopInventory('za')"
                    >
                        <v-list-item-title>Z-A</v-list-item-title>
                        <v-list-item-action>
                            <v-icon large>mdi-sort-alphabetical-descending</v-icon>
                        </v-list-item-action>
                    </v-list-item>
                </v-list>
            </v-menu>
          
        </v-app-bar>

        <v-navigation-drawer
            v-model="drawer"
            fixed
            temporary
        >
            <v-list
                nav
                class="pt-5"
            >
            
                <v-list-item 
                    @click="settingsStore.currentView = 'Inventory'; drawer = false;"
                    :input-value="settingsStore.currentView === 'Inventory'"
                >
                    <v-list-item-icon>
                        <v-icon>mdi-briefcase-outline</v-icon>
                    </v-list-item-icon>
                    <v-list-item-title>Inventory</v-list-item-title>
                </v-list-item>
            
                <v-list-item 
                    @click="settingsStore.currentView = 'Bazaar'; drawer = false;"
                    :input-value="settingsStore.currentView === 'Bazaar'"
                >
                    <v-list-item-icon>
                        <v-icon>mdi-basket-outline</v-icon>
                    </v-list-item-icon>
                    <v-list-item-title>Bazaar</v-list-item-title>
                </v-list-item>

                <v-slider
                    v-model="settingsStore.displayDensity.size"
                    :tick-labels="settingsStore.displayDensity.labels"
                    :max="2"
                    step="1"
                    ticks="always"
                    tick-size="3"
                    class="mb-4"
                ></v-slider>

                <v-list-item 
                    @click="addDialogStore.show = true; getFolderList('add');"
                    v-show="settingsStore.currentView === 'Inventory'"
                >
                    <v-list-item-icon>
                        <v-icon>mdi-plus</v-icon>
                    </v-list-item-icon>
                    <v-list-item-title>Add Item</v-list-item-title>
                </v-list-item>
                
                <!-- This is an example on how to make a custom add function. -->
                <v-list-item 
                    @click="bizCardDialogStore.show = true; getFolderList('add');"
                    v-show="false" 
                >
                    <v-list-item-icon>
                        <v-icon>mdi-plus</v-icon>
                    </v-list-item-icon>
                    <v-list-item-title>Create Business Card</v-list-item-title>
                </v-list-item>

                <v-list-item 
                    @click="createFolderDialogStore.show = true"
                    v-show="settingsStore.currentView === 'Inventory'"
                >
                    <v-list-item-icon>
                        <v-icon>mdi-folder-plus</v-icon>
                    </v-list-item-icon>
                    <v-list-item-title>Create Folder</v-list-item-title>
                </v-list-item>
                                
                <p class="app-version">Version {{ settingsStore.DASHBOARD_VERSION }}</p>

            </v-list>
        </v-navigation-drawer>
        
        <CategoryDrawer></CategoryDrawer>

        <v-main
            v-show="settingsStore.currentView === 'Inventory'"
        >
            <v-container fluid>
                <v-col
                    cols="12"
                    sm="6"
                    md="4"
                    lg="3"
                >
                    <InventoryItemIterator :itemsForIterator="this.$store.state.items"></InventoryItemIterator>
                </v-col>
            </v-container>
        </v-main>

        <v-main
            v-if="settingsStore.currentView === 'Bazaar'"
        >
            <Bazaar></Bazaar>
        </v-main>
        
        <v-bottom-navigation
            app
        >
            <span
                v-show="settingsStore.currentView === 'Bazaar'"
            >
                <v-select
                    :items="bazaarStore.categories"
                    item-text="title"
                    item-value="title"
                    label="Category"
                    class=""
                    @change="selectCategory"
                ></v-select>
            </span>
            <span
                v-show="settingsStore.currentView === 'Bazaar'"
            >
                <v-select
                    :items="bazaarStore.subCategories"
                    item-text="title"
                    item-value="title"
                    label="Subcategories"
                    class=""
                    @change="selectCategory"
                ></v-select>
            </span>
            <!-- <v-btn
                @click="bazaarStore.categoryDrawer = true"
                color="primary"
                class="mx-1"
            >
                <v-icon dark>
                    mdi-shape
                </v-icon>
            </v-btn> -->
            <!-- <div
                v-show="currentCategoryRecords > 0"
                class="ml-6 mt-4"
            >
                Loaded {{ currentCategoryRecordsLoaded }} of {{ currentCategoryRecords }} total items
            </div> -->
        </v-bottom-navigation>
        
        <!-- ### DIALOGS ### -->
        <v-dialog
            v-model="$store.state.removeDialog.show"
            max-width="290"
        >
            <transition name="fade" mode="out-in">
                <RemoveItem v-on:remove-item="removeItem"></RemoveItem>
            </transition>
        </v-dialog>
        
        <v-dialog
            v-model="$store.state.removeFolderDialog.show"
            max-width="290"
        >
            <transition name="fade" mode="out-in">
                <RemoveFolder v-on:remove-folder="removeFolder"></RemoveFolder>
            </transition>
        </v-dialog>
        
        <v-dialog
            v-model="$store.state.editDialog.show"
            max-width="380"
            :fullscreen="$store.state.settings.useFullscreenDialogs"
            hide-overlay
            transition="dialog-bottom-transition"
        >
            <transition name="fade" mode="out-in">
                <EditItem v-on:edit-item="editItem"></EditItem>
            </transition>
        </v-dialog>
        
        <v-dialog
            v-model="$store.state.editFolderDialog.show"
            max-width="380"
            :fullscreen="$store.state.settings.useFullscreenDialogs"
            hide-overlay
            transition="dialog-bottom-transition"
        >
            <transition name="fade" mode="out-in">
                <EditFolder v-on:edit-folder="editFolder"></EditFolder>
            </transition>
        </v-dialog>
        
        <v-dialog
            v-model="createFolderDialogStore.show"
            max-width="380"
            :fullscreen="$store.state.settings.useFullscreenDialogs"
            hide-overlay
            transition="dialog-bottom-transition"
        >
            <transition name="fade" mode="out-in">
                <CreateFolder v-on:create-folder="createFolder"></CreateFolder>
            </transition>
        </v-dialog>
        
        <v-dialog
            v-model="receiveDialogStore.show"
            max-width="380"
            persistent
            :fullscreen="$store.state.settings.useFullscreenDialogs"
            hide-overlay
            transition="dialog-bottom-transition"
        >
            <transition name="fade" mode="out-in">
                <ReceiveItem v-on:confirm-item-receipt="confirmItemReceipt"></ReceiveItem>
            </transition>
        </v-dialog>
        
        <v-dialog
            v-model="addDialogStore.show"
            max-width="380"
            :fullscreen="$store.state.settings.useFullscreenDialogs"
            hide-overlay
            transition="dialog-bottom-transition"
        >
            <transition name="fade" mode="out-in">
                <AddItem v-on:add-item="addItem"></AddItem>
            </transition>
        </v-dialog>
        
        <v-dialog
            v-model="$store.state.shareDialog.show"
            max-width="380"
            persistent
            :fullscreen="$store.state.settings.useFullscreenDialogs"
            hide-overlay
            transition="dialog-bottom-transition"
        >
            <transition name="fade" mode="out-in">
                <ShareItem v-on:share-item="shareItem"></ShareItem>
            </transition>
        </v-dialog>
        
        <v-dialog
            v-model="$store.state.itemPage.show"
            :fullscreen="$store.state.settings.useFullscreenDialogs"
            persistent
        >
            <transition name="fade" mode="out-in">
                <ItemPage></ItemPage>
            </transition>
        </v-dialog>
        
        <v-dialog
            v-model="receivingItemsDialogStore.show"
            max-width="380"
        >
            <transition name="fade" mode="out-in">
                <ReceivingItems 
                    v-on:accept-receiving-item="acceptReceivingItem"
                    v-on:remove-receiving-item="removeReceivingItem"
                ></ReceivingItems>
            </transition>
        </v-dialog>
        
        <!-- CUSTOM DIALOG -->
        <v-dialog
            v-model="bizCardDialogStore.show"
            max-width="380"
            :fullscreen="$store.state.settings.useFullscreenDialogs"
            hide-overlay
            transition="dialog-bottom-transition"
        >
            <v-card>
                <v-card-title class="headline">Create Business Card</v-card-title>

                <v-form
                    ref="bizCardForm"
                    v-model="bizCardDialogStore.valid"
                    :lazy-validation="false"
                >

                    <v-card-text>
                        Name your business card.
                    </v-card-text>

                    <v-text-field
                        class="px-2"
                        label="Name of Business Card"
                        v-model="bizCardDialogStore.data.name"
                        :rules="[v => !!v || 'Name is required.']"
                        required
                    ></v-text-field>

                    <v-text-field
                        label="Full Name"
                        outlined
                        v-model="bizCardDialogStore.data.fullName"
                    ></v-text-field>

                    <v-text-field
                        label="Title"
                        outlined
                        v-model="bizCardDialogStore.data.title"
                    ></v-text-field>
                    
                    <v-text-field
                        label="Company"
                        outlined
                        v-model="bizCardDialogStore.data.company"
                    ></v-text-field>

                    <v-text-field
                        label="E-mail"
                        outlined
                        v-model="bizCardDialogStore.data.email"
                    ></v-text-field>

                    <v-text-field
                        label="Phone Number"
                        outlined
                        v-model="bizCardDialogStore.data.phoneNumber"
                    ></v-text-field>

                    <v-text-field
                        label="Website"
                        outlined
                        v-model="bizCardDialogStore.data.website"
                    ></v-text-field>

                    <v-textarea
                        label="Details"
                        outlined
                        v-model="bizCardDialogStore.data.details"
                    ></v-textarea>

                    <!-- FIXME: Is this going to be hard to integrate since it's add specific or..? -->
                    <v-card-text>
                        Select a folder (optional).
                    </v-card-text>

                    <v-select
                        class="my-2"
                        :items="folderList"
                        v-model="bizCardDialogStore.data.folder"
                        label="Folder"
                        outlined
                        item-text="name"
                        item-value="uuid"
                    ></v-select>

                    <v-card-actions>

                        <v-btn
                            color="red"
                            class="px-3"
                            @click="bizCardDialogStore.show = false"
                        >
                            Cancel
                        </v-btn>

                        <v-spacer></v-spacer>

                        <v-btn
                            color="blue"
                            class="px-3"
                            :disabled="!$store.state.bizCardDialog.valid"
                            @click="bizCardDialogStore.show = false; addBusinessCard()"
                        >
                            Create
                        </v-btn>

                    </v-card-actions>

                </v-form>
            </v-card>
        </v-dialog>

    </v-app>
</template>

<script>
// Components
import InventoryItemIterator from './components/InventoryItemIterator'
import Bazaar from './components/Bazaar'
import CategoryDrawer from './components/CategoryDrawer'
// Components -> Dialogs
import AddItem from './components/Dialogs/AddItem'
import CreateFolder from './components/Dialogs/CreateFolder'
import EditFolder from './components/Dialogs/EditFolder'
import EditItem from './components/Dialogs/EditItem'
import ItemPage from './components/Dialogs/ItemPage'
import ReceiveItem from './components/Dialogs/ReceiveItem'
import ReceivingItems from './components/Dialogs/ReceivingItems'
import RemoveFolder from './components/Dialogs/RemoveFolder'
import RemoveItem from './components/Dialogs/RemoveItem'
import ShareItem from './components/Dialogs/ShareItem'
// Plugins
import { EventBus } from './plugins/event-bus.js';

var vue_this;

function browserDevelopment() {
    if (typeof EventBridge !== 'undefined') {
        return false; // We are in Vircadia.
    } else {
        return true; // We are in the browser, probably for development purposes.
    }
}

if (!browserDevelopment()) {
    // eslint-disable-next-line
    EventBridge.scriptEventReceived.connect(function(receivedCommand) {
        receivedCommand = JSON.parse(receivedCommand);
        // alert("RECEIVED COMMAND:" + receivedCommand.command)
        if (receivedCommand.app === "inventory") {
            // We route the data based on the command given.
            switch (receivedCommand.command) {
                case 'script-to-web-inventory':
                    // alert("INVENTORY RECEIVED ON APP:" + JSON.stringify(receivedCommand.data));
                    vue_this.receiveInventory(receivedCommand.data);
                    break;
                case 'script-to-web-receiving-item-queue':
                    // alert("RECEIVING ITEM QUEUE:" + JSON.stringify(receivedCommand.data));
                    vue_this.receiveReceivingItemQueue(receivedCommand.data);
                    break;
                case 'script-to-web-nearby-users':
                    // alert("RECEIVING NEARBY USERS:" + JSON.stringify(receivedCommand.data));
                    vue_this.receiveNearbyUsers(receivedCommand.data);
                    break;
                case 'script-to-web-settings':
                    // alert("RECEIVING SETTINGS:" + JSON.stringify(receivedCommand.data));
                    vue_this.receiveSettings(receivedCommand.data);
                    break;
                case 'script-to-web-append-item':
                    // alert("RECEIVING ITEM APPEND:" + JSON.stringify(receivedCommand.data));
                    vue_this.receiveAppendItem(receivedCommand.data);
                    break;
            }
        }
    });
    
}

EventBus.$on('use-item', data => {
    vue_this.useItem(data.type, data.url);
});

EventBus.$on('add-item-from-bazaar', data => {
    vue_this.addDialogStore.data.name = data.name;
    vue_this.addDialogStore.data.folder = null;
    vue_this.addDialogStore.data.url = data.main;
    vue_this.addDialogStore.data.tags = data.tags;
    vue_this.addDialogStore.data.metadata = data.description;
    vue_this.addDialogStore.data.version = data.version;
    vue_this.addDialogStore.show = true; 
    vue_this.getFolderList('add');
});

export default {
    name: 'App',
    components: {
        Bazaar,
        CategoryDrawer,
        InventoryItemIterator,
        // Dialogs
        AddItem,
        CreateFolder,
        EditFolder,
        EditItem,
        ItemPage,
        ReceiveItem,
        ReceivingItems,
        RemoveItem,
        RemoveFolder,
        ShareItem
    },
    data: () => ({
        recursiveFolderHoldingList: [],
        drawer: false
    }),
    created: function () {
        vue_this = this;
        this.$vuetify.theme.dark = this.settingsStore.darkTheme;
                
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
        pushToItems: function(type, name, folder, url, tags, metadata, version, uuid) {
            var uuidToUse;

            if (uuid != null) {
                uuidToUse = uuid;
            } else {
                uuidToUse = this.createUUID();
            }
            
            this.$store.commit('pushToItems', {
                "type": type,
                "name": name,
                "folder": folder,
                "url": url,
                "tags": tags,
                "metadata": metadata,
                "version": version,
                "uuid": uuidToUse
            });
            
            if (folder !== null && folder !== "No Folder") {
                this.moveItem(uuidToUse, folder);
            }
        },
        checkFileType: function(fileType) {
            var detectedItemType = null;
            
            switch (fileType) {
                // Model Cases
                case ".fbx":
                    detectedItemType = "MODEL";
                    break;
                case ".gltf":
                    detectedItemType = "MODEL";
                    break;
                case ".glb":
                    detectedItemType = "MODEL";
                    break;
                // Script Cases
                case ".js":
                    detectedItemType = "SCRIPT";
                    break;
                // Avatar Cases
                case ".fst":
                    detectedItemType = "AVATAR";
                    break;
                // JSON Cases
                case ".json":
                    detectedItemType = "JSON";
                    break;
                default:
                    detectedItemType = "OTHER";
            }
            
            return detectedItemType;
        },
        checkItemType: function (itemType) {
            var detectedItemType = null;
            itemType = itemType.toUpperCase();
            
            this.$store.state.supportedItemTypes.forEach(function (itemTypeInList) {
                if (itemTypeInList === itemType) {
                    detectedItemType = itemTypeInList;
                }
            });
            
            if (detectedItemType === null) {
                // This is not a known item type...
                detectedItemType = "OTHER";
            }
            
            return detectedItemType;
        },
        detectFileType: function (url) {    
            // Attempt the pure regex route...
            var extensionRegex = /\.[0-9a-z]+$/i; // to detect the file type based on extension in the URL.
            var detectedFileType = url.match(extensionRegex);

            // If that fails, let's try the traditional URL route.
            if (detectedFileType == null || detectedFileType[0] == null) {
                var urlExtensionRegex = /\.[0-9a-z]+$/i;
                var urlToParse;
                
                // Attempt to construct a URL from the supposed URL.
                try {
                    urlToParse = new URL(url);
                } catch (e) {
                    return null;
                }

                // Attempt the URL converted regex route...
                detectedFileType = urlToParse.pathname.match(urlExtensionRegex);
            } else if (detectedFileType == null || detectedFileType[0] == null) { // Still not working?!
                // Your URL sucks!
                detectedFileType = null; // We got nothin'.
            }
            
            return detectedFileType;
        },
        createFolder: function (name) {
            this.$store.commit('pushToItems', {
                "name": name,
                "folder": "No Folder",
                "items": [],
                "uuid": this.createUUID()
            });

            this.createFolderDialogStore.data.name = null;
        },
        editFolder: function (uuid) {
            var findFolder = this.searchForItem(uuid);
            
            if (findFolder) {
                findFolder.returnedItem.name = this.$store.state.editFolderDialog.data.name;
                
                if (this.$store.state.editFolderDialog.data.folder !== null && this.$store.state.editFolderDialog.data.folder !== "No Change") {
                    if (findFolder.returnedItem.folder !== this.$store.state.editFolderDialog.data.folder && this.$store.state.editFolderDialog.data.folder !== "No Folder") {
                        this.moveFolder(uuid, this.$store.state.editFolderDialog.data.folder);
                    } else if (this.$store.state.editFolderDialog.data.folder === "No Folder") {
                        this.moveFolder(uuid, "top");
                    }
                }
            }
        },
        addItem: function () {
            var name = this.$store.state.addDialog.data.name;
            var folder = this.$store.state.addDialog.data.folder;
            var url = this.$store.state.addDialog.data.url;
            var tags = this.$store.state.addDialog.data.tags;
            var metadata = this.$store.state.addDialog.data.metadata;
            var version = this.$store.state.addDialog.data.version;
            
            var detectedFileType = this.detectFileType(url);
            var itemType;
                        
            if (detectedFileType == null || detectedFileType[0] == null) {
                itemType = "OTHER";
            } else {
                itemType = this.checkFileType(detectedFileType[0]);
            }

            this.pushToItems(itemType, name, folder, url, tags, metadata, version, null);
            
            this.addDialogStore.data.name = null;
            this.addDialogStore.data.folder = null;
            this.addDialogStore.data.url = null;
        
        },
        removeItem: function (uuid) {
            var findItem = this.searchForItem(uuid);
            findItem.parentArray.splice(findItem.iteration, 1);
        },
        removeFolder: function (uuid) {
            var findFolder = this.searchForItem(uuid);
            findFolder.parentArray.splice(findFolder.iteration, 1);
        },
        editItem: function (uuid) {    
            var findItem = this.searchForItem(uuid);
            findItem.returnedItem.type = this.checkItemType(this.$store.state.editDialog.data.type);
            findItem.returnedItem.name = this.$store.state.editDialog.data.name;
            findItem.returnedItem.folder = this.$store.state.editDialog.data.folder;
            findItem.returnedItem.url = this.$store.state.editDialog.data.url;
            findItem.returnedItem.tags = this.$store.state.editDialog.data.tags;
            findItem.returnedItem.metadata = this.$store.state.editDialog.data.metadata;
            findItem.returnedItem.version = this.$store.state.editDialog.data.version;

            var folderName;

            for (var i = 0; i < this.folderList.length; i++) {
                if (this.folderList[i].name === findItem.returnedItem.folder) {
                    folderName = this.folderList[i].name;
                }
            }

            if (this.$store.state.editDialog.data.folder !== null) {
                if (folderName !== this.$store.state.editDialog.data.folder && this.$store.state.editDialog.data.folder !== "No Folder") {
                    this.moveItem(uuid, this.$store.state.editDialog.data.folder);
                } else if (folderName === "No Folder") {
                    this.moveItem(uuid, "top");
                }
            }
        },
        acceptReceivingItem: function (data) {
            this.removeReceivingItem(data.data.uuid);
            
            this.receiveDialogStore.data.userUUID = data.senderUUID;
            this.receiveDialogStore.data.userDisplayName = data.senderName;
            this.receiveDialogStore.data.type = data.data.type;
            this.receiveDialogStore.data.name = data.data.name;
            this.receiveDialogStore.data.url = data.data.url;
            this.receiveDialogStore.data.tags = data.data.tags;
            this.receiveDialogStore.data.metadata = data.data.metadata;
            this.receiveDialogStore.data.version = data.data.version;
            
            this.getFolderList("add");
                            
            this.receiveDialogStore.show = true;
        },
        removeReceivingItem: function (uuid) {
            for (var i = 0; i < this.receivingItemsDialogStore.data.receivingItemQueue.length; i++) {
                if (this.receivingItemsDialogStore.data.receivingItemQueue[i].data.uuid === uuid) {
                    this.receivingItemsDialogStore.data.receivingItemQueue.splice(i, 1);
                    this.sendAppMessage('web-to-script-update-receiving-item-queue', this.receivingItemsDialogStore.data.receivingItemQueue);
                    if (this.receivingItemsDialogStore.data.receivingItemQueue.length === 0) {
                        this.receivingItemsDialogStore.show = false; // Close the dialog if there's nothing left.
                    }
                }
            }
        },
        receiveReceivingItemQueue: function (data) {
            this.receivingItemsDialogStore.data.receivingItemQueue = data;
        },
        receiveAppendItem: function (data) {
            this.pushToItems(data.type, data.name, "No Folder", data.url, data.tags, data.metadata, data.version, null);
        },
        shareItem: function (uuid) {        
            var findItem = this.searchForItem(uuid);
            var nameToShare = findItem.returnedItem.name;
            var typeToShare = findItem.returnedItem.type;
            var tagsToShare = findItem.returnedItem.tags;
            var metadataToShare = findItem.returnedItem.metadata;
            var versionToShare = findItem.returnedItem.version;
            
            // alert("type" + typeToShare + "name" + nameToShare);
            this.sendAppMessage("share-item", {
                "type": typeToShare,
                "name": nameToShare,
                "tags": tagsToShare,
                "metadata": metadataToShare,
                "version": versionToShare,
                "url": this.$store.state.shareDialog.data.url,
                "recipient": this.$store.state.shareDialog.data.recipient,
            });
        },
        confirmItemReceipt: function () {
            this.pushToItems(
                this.checkItemType(this.$store.state.receiveDialog.data.type), 
                this.$store.state.receiveDialog.data.name,
                this.$store.state.receiveDialog.data.folder,
                this.$store.state.receiveDialog.data.url,
                this.$store.state.receiveDialog.data.tags,
                this.$store.state.receiveDialog.data.metadata,
                this.$store.state.receiveDialog.data.version,
                null
            );
        },
        useItem: function (type, url) {
            this.sendAppMessage("use-item", { 
                "type": type, 
                "url": url 
            });
        },
        onDragStart: function () {
            console.info("Drag start.");
        },
        onDragUpdate: function () {
            console.info("Drag Update.");
        },
        onDragEnd: function () {
            console.info("Drag End.");
        },
        onDragChange: function (ev) {
            console.info("Drag Update.", ev);
        },
        sortTopInventory: function (order) {
            this.$store.commit('sortTopInventory', { "sort": order });
        },
        getFolderList: function (request) {
            var generateList;
            this.recursiveFolderHoldingList = []; // Clear that list before we do anything.
            
            if (request === "edit") {
                this.folderList = [
                    {
                        "name": "No Change",
                        "uuid": "No Change"
                    },
                    {
                        "name": "No Folder", 
                        "uuid": "No Folder"
                    }
                ];
                
                generateList = this.recursiveFolderPopulate(this.itemsStore, null);
                
            } else if (request === "add") {
                this.folderList = [
                    {
                        "name": "No Folder", 
                        "uuid": "No Folder"
                    }
                ];
                
                generateList = this.recursiveFolderPopulate(this.itemsStore, null);
                
            } else if (request === "editFolder") {
                this.folderList = [
                    {
                        "name": "No Change",
                        "uuid": "No Change"
                    },
                    {
                        "name": "No Folder", 
                        "uuid": "No Folder"
                    }
                ];
                
                generateList = this.recursiveFolderPopulate(this.itemsStore, this.$store.state.editFolderDialog.data.uuid);
            }
            
            if (generateList) {
                var combinedArray = this.folderList.concat(generateList);
                this.folderList = combinedArray;
            }
        },
        moveItem: function (uuid, parentFolderUUID) {
            var findItem = this.searchForItem(uuid);
            var findParentFolder;
            
            if (parentFolderUUID !== "top") {
                findParentFolder = this.searchForItem(parentFolderUUID);
            }
            
            // Remove the old item before placing down the copy, we already got the attributes that we had wanted.
            this.removeItem(uuid);
            
            this.$store.commit('moveItem', {
                "uuid": uuid,
                "parentFolderUUID": parentFolderUUID,
                "findItem": findItem,
                "findParentFolder": findParentFolder
            });

        },
        moveFolder: function (uuid, parentFolderUUID) {
            var findFolder = this.searchForItem(uuid);
            var findParentFolder;
            
            if (parentFolderUUID !== "top") {
                findParentFolder = this.searchForItem(parentFolderUUID);
            }
            
            // Remove the old item before placing down the copy, we already got the attributes that we had wanted.
            this.removeFolder(uuid);
            
            this.$store.commit('moveFolder', {
                "uuid": uuid,
                "parentFolderUUID": parentFolderUUID,
                "findFolder": findFolder,
                "findParentFolder": findParentFolder
            });
            
        },
        searchForItem: function (uuid) {
            var foundItem = this.recursiveSingularSearch(uuid, this.itemsStore);
            
            if (foundItem) {
                return {
                    "returnedItem": foundItem.returnedItem,
                    "iteration": foundItem.iteration,
                    "parentArray": foundItem.parentArray,
                    "itemUUID": uuid
                }
            }
        },
        recursiveSingularSearch: function (uuid, indexToSearch) {
            for (var i = 0; i < indexToSearch.length; i++) {
                if (indexToSearch[i].uuid === uuid) {
                    var foundItem = {
                        "returnedItem": indexToSearch[i],
                        "iteration": i,
                        "parentArray": indexToSearch
                    }
                    return foundItem;
                } else if (Object.prototype.hasOwnProperty.call(indexToSearch[i], "items") && indexToSearch[i].items.length > 0) {
                    var deepSearch = this.recursiveSingularSearch(uuid, indexToSearch[i].items);
                    if (deepSearch !== null) {
                        return deepSearch;
                    }
                }
            }
            
            return null;
        },
        recursiveFolderPopulate: function (indexToSearch, avoidFolder) {
            for (var i = 0; i < indexToSearch.length; i++) {
                if (Object.prototype.hasOwnProperty.call(indexToSearch[i], "items")) {
                    // We want to avoid adding the folder itself and also any child folders it may have, putting a folder within its child folder will nuke it.
                    if (avoidFolder !== indexToSearch[i].uuid) {
                        // console.info("AvoidFolder", avoidFolder, "indexToSearch[i].uuid", indexToSearch[i].uuid);
                        this.recursiveFolderHoldingList.push({
                            "name": indexToSearch[i].name,
                            "uuid": indexToSearch[i].uuid
                        });
                        
                        this.recursiveFolderPopulate(indexToSearch[i].items, avoidFolder);
                    }
                }
            }
            
            return this.recursiveFolderHoldingList;
        },
        selectCategory: function (category) {
            this.bazaarStore.selectedCategory = category;
        },
        
        // MAIN APP FUNCTIONS

        sendInventory: function () {
            this.sendAppMessage("web-to-script-inventory", this.itemsStore );
        },
        receiveInventory: function(receivedInventory) {
            if (!receivedInventory) {
                this.itemsStore = [];
            } else {
                this.itemsStore = receivedInventory;
            }
        },
        sendSettings: function () {
            this.sendAppMessage("web-to-script-settings", this.$store.state.settings );
        },
        receiveSettings: function (receivedSettings) {
            if (!receivedSettings) {
                // Don't do anything, let the defaults stand. Otherwise, it will break the app.
            } else {
                this.settings = receivedSettings;
            }
        },
        receiveNearbyUsers: function (receivedUsers) {
            if (!receivedUsers) {
                this.$store.commit('mutate', {
                    update: true,
                    property: 'nearbyUsers', 
                    with: []
                });
            } else {
                this.$store.commit('mutate', {
                    update: true,
                    property: 'nearbyUsers', 
                    with: receivedUsers
                });
            }
        },
        sendAppMessage: function (command, data) {
            var JSONtoSend = {
                "app": "inventory",
                "command": command,
                "data": data
            };
                        
            if (!browserDevelopment()) {
                // eslint-disable-next-line
                EventBridge.emitWebEvent(JSON.stringify(JSONtoSend));
            } else {
                // alert(JSON.stringify(JSONtoSend));
            }
        },
        sendEvent: function (command, data) {
            EventBus.$emit(command, data);
        },
        // --- CUSTOM METHODS ---
        addBusinessCard: function () {
            var name = this.$store.state.bizCardDialog.data.name;
            var folder = this.$store.state.bizCardDialog.data.folder;
            var url = "#";
            var tags = ["business-card"];
            var metadata = JSON.stringify({
                "fullName": this.$store.state.bizCardDialog.data.fullName,
                "title": this.$store.state.bizCardDialog.data.title,
                "company": this.$store.state.bizCardDialog.data.company,
                "email": this.$store.state.bizCardDialog.data.email,
                "phoneNumber": this.$store.state.bizCardDialog.data.phoneNumber,
                "website": this.$store.state.bizCardDialog.data.website,
                "details": this.$store.state.bizCardDialog.data.details
            });
            var version = '0.0.1';

            var itemType = "OTHER";

            this.pushToItems(itemType, name, folder, url, tags, metadata, version, null);
            
            this.bizCardDialogStore.data.name = null;
            this.bizCardDialogStore.data.folder = null;
            this.bizCardDialogStore.data.fullName = null;
            this.bizCardDialogStore.data.title = null;
            this.bizCardDialogStore.data.company = null;
            this.bizCardDialogStore.data.email = null;
            this.bizCardDialogStore.data.phoneNumber = null;
            this.bizCardDialogStore.data.website = null;
            this.bizCardDialogStore.data.details = null;
        },
    },
    computed: {
        itemsStore: {
            get() {
                return this.$store.state.items;
            },
            set (value) {
                this.$store.commit('mutate', {
                    property: 'items', 
                    with: value
                });
            }
        },
        folderList: {
            get() {
                return this.$store.state.folderList;
            },
            set (value) {
                this.$store.commit('mutate', {
                    property: 'folderList', 
                    with: value
                });
            }
        },
        addDialogStore: {
            get() {
                return this.$store.state.addDialog;
            },
            set (value) {
                this.$store.commit('mutate', {
                    property: 'addDialog', 
                    with: value
                });
            }
        },
        bazaarStore: {
            get() {
                return this.$store.state.bazaar;
            },
            set (value) {
                this.$store.commit('mutate', {
                    property: 'bazaar', 
                    with: value
                });
            }
        },
        editDialogShow: function() {
            return this.$store.state.editDialog.show;
        },
        editFolderDialogShow: function() {
            return this.$store.state.editFolderDialog.show;
        },
        createFolderDialogStore: {
            get() {
                return this.$store.state.createFolderDialog;
            },
            set (value) {
                this.$store.commit('mutate', {
                    property: 'createFolderDialog', 
                    with: value
                });
            }
        },
        receiveDialogStore: {
            get() {
                return this.$store.state.receiveDialog;
            },
            set (value) {
                this.$store.commit('mutate', {
                    property: 'receiveDialog', 
                    with: value
                });
            }
        },
        shareDialogShow: function() {
            return this.$store.state.shareDialog.show;
        },
        receivingItemsDialogStore: {
            get() {
                return this.$store.state.receivingItemsDialog;
            },
            set (value) {
                this.$store.commit('mutate', {
                    property: 'receivingItemsDialog', 
                    with: value
                });
            }
        },
        searchBoxStore: {
            get () {
                return this.$store.state.searchBox;
            },
            set (value) {
                this.$store.commit('mutate', {
                    property: 'searchBox', 
                    with: value
                });
            }
        },
        settingsStore: {
            get () {
                return this.$store.state.settings;
            },
            set (value) {
                this.$store.commit('mutate', {
                    property: 'settings', 
                    with: value
                });
            }
        },
        settingsStoreCurrentView: function () {
            return this.$store.state.settings.currentView;
        },
        // --- CUSTOM DATA ---
        bizCardDialogStore: {
            get () {
                return this.$store.state.bizCardDialog;
            },
            set (value) {
                this.$store.commit('mutate', {
                    property: 'bizCardDialog', 
                    with: value
                });
            }
        }
    },
    watch: {
        // Whenever the item list changes, this will notice and then send it to the script to be saved.
        itemsStore: {
            deep: true,
            handler() {
                this.sendInventory();
            }
        }, 
        // Whenever the settings change, we want to save that state.
        settingsStore: {
            deep: true,
            handler: function() {
                this.sendSettings();
            }
        },
        editDialogShow: {
            handler: function(newVal) {
                if (newVal === true) {
                    this.getFolderList('edit');
                }
            }
        },
        editFolderDialogShow: {
            handler: function(newVal) {
                if (newVal === true) {
                    this.getFolderList('editFolder');
                }
            }
        },
        shareDialogShow: {
            handler: function(newVal) {
                if (newVal === true) {
                    this.sendAppMessage('web-to-script-request-nearby-users', '');
                }
            }
        },
        settingsStoreCurrentView: {
            handler: function() {
                // When the view changes between the Inventory and Bazaar do the following...
                this.searchBoxStore = '';
            }
        }
    }
};

</script>
