<!--
//
//  App.vue
//
//  Created by kasenvr@gmail.com on 7 Apr 2020
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
        <v-app-bar
            app
        >
            <v-app-bar-nav-icon @click="drawer = true"></v-app-bar-nav-icon>

            <v-toolbar-title>Inventory</v-toolbar-title>
            
            <v-spacer></v-spacer>
            
            <v-btn medium color="primary" fab @click="sortTopInventory('top')">
                <v-icon>
                    mdi-ab-testing
                </v-icon>
            </v-btn>
          
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
                <v-list-item-group>

                    <v-slider
                        v-model="settings.displayDensity.size"
                        :tick-labels="settings.displayDensity.labels"
                        :max="2"
                        step="1"
                        ticks="always"
                        tick-size="3"
                    ></v-slider>

                    <v-list-item @click="addDialogStore.show = true; getFolderList('add');">
                        <v-list-item-icon>
                            <v-icon>mdi-plus</v-icon>
                        </v-list-item-icon>
                        <v-list-item-title>Add Item</v-list-item-title>
                    </v-list-item>

                    <v-list-item @click="createFolderDialogStore.show = true">
                        <v-list-item-icon>
                            <v-icon>mdi-folder-plus</v-icon>
                        </v-list-item-icon>
                        <v-list-item-title>Create Folder</v-list-item-title>
                    </v-list-item>
                                    
                    <p class="app-version">Version {{appVersion}}</p>

                </v-list-item-group>
            </v-list>
        </v-navigation-drawer>

        <v-content>
            <v-container fluid>
                <v-col
                    cols="12"
                    sm="6"
                    md="4"
                    lg="3"
                    class="py-1 column-item"
                >
                    <itemiterator :itemsForIterator="itemsStore"></itemiterator>
                </v-col>
            </v-container>
        </v-content>

        <v-dialog
          v-model="removeDialogStore.show"
          max-width="290"
        >
          <v-card>
              <v-card-title class="headline">Remove Item</v-card-title>

              <v-card-text>
                  Are you sure you want to delete this item from your inventory?
              </v-card-text>

              <v-card-actions>

                  <v-btn
                      color="blue"
                      class="px-3"
                      @click="removeDialogStore.show = false"
                  >
                      No
                  </v-btn>
                  
                  <v-spacer></v-spacer>
                  
                  <v-btn
                      color="red"
                      class="px-3"                    
                      @click="removeDialogStore.show = false; removeItem($store.state.removeDialog.uuid);"
                  >
                      Yes
                  </v-btn>
                  
              </v-card-actions>
              
          </v-card>
        </v-dialog>
        
        <v-dialog
          v-model="removeFolderDialogStore.show"
          max-width="290"
        >
          <v-card>
              <v-card-title class="headline">Remove Folder</v-card-title>

              <v-card-text>
                  Are you sure you want to delete this folder <b>and</b> all items within from your inventory?
              </v-card-text>

              <v-card-actions>

                  <v-btn
                      color="blue"
                      class="px-3"
                      @click="removeFolderDialogStore.show = false"
                  >
                      No
                  </v-btn>
                  
                  <v-spacer></v-spacer>
                  
                  <v-btn
                      color="red"
                      class="px-3"                    
                      @click="removeFolderDialogStore.show = false; removeFolder($store.state.removeFolderDialog.uuid);"
                  >
                      Yes
                  </v-btn>
                  
              </v-card-actions>
              
          </v-card>
        </v-dialog>

        <v-dialog
          v-model="editDialogStore.show"
          max-width="380"
        >
          <v-card>
              <v-card-title class="headline">Edit Item</v-card-title>
              
              <v-form
                  ref="editForm"
                  v-model="editDialogStore.valid"
                  :lazy-validation="false"
              >
                    
                    <v-select
                        :items="$store.state.supportedItemTypes"
                        class="my-2"
                        v-model="editDialogStore.data.type"
                        :rules="[v => !!v || 'Type is required.']"
                        label="Item Type"
                        outlined
                    ></v-select>

                    <v-text-field
                        class="px-2"
                        label="Name"
                        v-model="editDialogStore.data.name"
                        :rules="[v => !!v || 'Name is required.']"
                        required
                    ></v-text-field>

                    <v-select
                        :items="folderList"
                        item-text="name"
                        item-value="uuid"
                        class="my-2"
                        v-model="editDialogStore.data.folder"
                        label="Folder"
                        outlined
                    ></v-select>

                    <v-text-field
                        class="px-2"
                        label="URL"
                        v-model="editDialogStore.data.url"
                        :rules="[v => !!v || 'URL is required.']"
                        required
                    ></v-text-field>

                  <v-card-actions>

                      <v-btn
                          color="red"
                          class="px-3"
                          @click="editDialogStore.show = false"
                      >
                          Cancel
                      </v-btn>
                      
                      <v-spacer></v-spacer>
                      
                      <v-btn
                          color="blue"
                          class="px-3"       
                          :disabled="!$store.state.editDialog.valid"             
                          @click="editDialogStore.show = false; editItem($store.state.editDialog.uuid);"
                      >
                          Done
                      </v-btn>
                  
                  </v-card-actions>
                  
              </v-form>

          </v-card>
        </v-dialog>
        
        <v-dialog
          v-model="editFolderDialogStore.show"
          max-width="380"
        >
          <v-card>
              <v-card-title class="headline">Edit Folder</v-card-title>
              
              <v-form
                  ref="editFolderForm"
                  v-model="editFolderDialogStore.valid"
                  :lazy-validation="false"
              >

                  <v-text-field
                      class="px-2"
                      label="Name"
                      v-model="editFolderDialogStore.data.name"
                      :rules="[v => !!v || 'Name is required.']"
                      required
                  ></v-text-field>
                  
                  <v-select
                      :items="folderList"
                      item-text="name"
                      item-value="uuid"
                      class="my-2"
                      v-model="editFolderDialogStore.data.folder"
                      label="Parent Folder"
                      outlined
                  ></v-select>

                  <v-card-actions>

                      <v-btn
                          color="red"
                          class="px-3"
                          @click="editFolderDialogStore.show = false"
                      >
                          Cancel
                      </v-btn>
                      
                      <v-spacer></v-spacer>
                      
                      <v-btn
                          color="blue"
                          class="px-3"       
                          :disabled="!$store.state.editFolderDialog.valid"             
                          @click="editFolderDialogStore.show = false; editFolder($store.state.editFolderDialog.data.uuid);"
                      >
                          Done
                      </v-btn>
                  
                  </v-card-actions>
                  
              </v-form>

          </v-card>
        </v-dialog>

        <v-dialog
          v-model="createFolderDialogStore.show"
          max-width="380"
        >
          <v-card>
              <v-card-title class="headline">Create Folder</v-card-title>
              
              <v-card-text>
                  Enter the name of the folder.
              </v-card-text>
              
              <v-form
                  ref="createFolderForm"
                  v-model="createFolderDialogStore.valid"
                  :lazy-validation="false"
              >

                  <v-text-field
                      class="px-2"
                      label="Name"
                      v-model="createFolderDialogStore.data.name"
                      :rules="[v => !!v || 'Name is required.']"
                      required
                  ></v-text-field>

                  <v-card-actions>

                      <v-btn
                          color="red"
                          class="px-3"
                          @click="createFolderDialogStore.show = false"
                      >
                          Cancel
                      </v-btn>
                      
                      <v-spacer></v-spacer>
                      
                      <v-btn
                          color="blue"
                          class="px-3"
                          :disabled="!$store.state.createFolderDialog.valid"
                          @click="createFolderDialogStore.show = false; createFolder($store.state.createFolderDialog.data.name)"
                      >
                          Create
                      </v-btn>
                      
                  </v-card-actions>
              
              </v-form>
          </v-card>
        </v-dialog>

        <v-dialog
          v-model="addDialogStore.show"
          max-width="380"
        >
          <v-card>
              <v-card-title class="headline">Add Item</v-card-title>
             
              
              <v-form
                  ref="addForm"
                  v-model="addDialogStore.valid"
                  :lazy-validation="false"
              >
              
                  <v-card-text>
                      Enter the name of the item.
                  </v-card-text>

                  <v-text-field
                      class="px-2"
                      label="Name"
                      v-model="addDialogStore.data.name"
                      :rules="[v => !!v || 'Name is required.']"
                      required
                  ></v-text-field>
                  
                  <v-card-text>
                      Select a folder (optional).
                  </v-card-text>
                  
                  <v-select
                      class="my-2"
                      :items="folderList"
                      v-model="addDialogStore.data.folder"
                      label="Folder"
                      outlined
                      item-text="name"
                      item-value="uuid"
                  ></v-select>

                  <v-card-text>
                      Enter the URL of the item.
                  </v-card-text>

                  <v-text-field
                      class="px-2"
                      label="URL"
                      v-model="addDialogStore.data.url"
                      :rules="[v => !!v || 'URL is required.']"
                      required
                  ></v-text-field>

                  <v-card-actions>

                      <v-btn
                          color="red"
                          class="px-3"
                          @click="addDialogStore.show = false"
                      >
                          Cancel
                      </v-btn>
                      
                      <v-spacer></v-spacer>
                      
                      <v-btn
                          color="blue"
                          class="px-3"
                          :disabled="!$store.state.addDialog.valid"
                          @click="addDialogStore.show = false; addItem($store.state.addDialog.data.name, $store.state.addDialog.data.folder, $store.state.addDialog.data.url)"
                      >
                          Add
                      </v-btn>
                      
                  </v-card-actions>
              
              </v-form>
          </v-card>
        </v-dialog>

        <v-dialog
          v-model="receiveDialogStore.show"
          max-width="380"
          persistent
        >
          <v-card>
              <v-card-title class="headline">Receiving Item</v-card-title>

              <v-card-text>
                  {{$store.state.receiveDialog.data.user}} is sending you an item.
              </v-card-text>
              
              <v-form
                  ref="receiveForm"
                  v-model="receiveDialogStore.valid"
                  :lazy-validation="false"
              >
              
                  <v-text-field
                      class="px-2"
                      label="Type"
                      :rules="[v => !!v || 'Type is required.']"
                      v-model="receiveDialogStore.data.type"
                      required
                  ></v-text-field>
                  
                  <v-text-field
                      class="px-2"
                      label="Name"
                      :rules="[v => !!v || 'Name is required.']"
                      v-model="receiveDialogStore.data.name"
                      required
                  ></v-text-field>
                  
                  <v-card-text>
                      Select a folder (optional).
                  </v-card-text>
                  
                  <v-select
                      class="my-2"
                      :items="folderList"
                      v-model="receiveDialogStore.data.folder"
                      label="Folder"
                      outlined
                      item-text="name"
                      item-value="uuid"
                  ></v-select>

                  <v-text-field
                      class="px-2"
                      label="URL"
                      :rules="[v => !!v || 'URL is required.']"
                      v-model="receiveDialogStore.data.url"
                      required
                  ></v-text-field>

                  <v-card-actions>

                      <v-btn
                          color="red"
                          class="px-3"
                          @click="receiveDialogStore.show = false"
                      >
                          Reject
                      </v-btn>
                      
                      <v-spacer></v-spacer>
                      
                      <v-btn
                          color="blue"
                          class="px-3"
                          :disabled="!$store.state.receiveDialog.valid"
                          @click="receiveDialogStore.show = false; acceptItem();"
                      >
                          Accept
                      </v-btn>
                      
                  </v-card-actions>
                  
              </v-form>
          </v-card>
        </v-dialog>

        <v-dialog
          v-model="shareDialogStore.show"
          max-width="380"
          persistent
        >
          <v-card>
              <v-card-title class="headline">Share Item</v-card-title>

              <v-card-text>
                  Select a user to send this item to.
              </v-card-text>
              
              <v-form
                  ref="shareForm"
                  v-model="shareDialogStore.valid"
                  :lazy-validation="false"
                  class="px-2"
              >
              
                  <!-- <v-list>
                      <v-list-item-group v-model="shareDialogStore.data.recipient" color="primary">
                          <v-list-item
                              v-for="user in nearbyUsers"
                              v-bind:key="user.uuid"
                          >
                              <v-list-item-content>
                                  <v-list-item-title v-text="user.name"></v-list-item-title>
                              </v-list-item-content>
                          </v-list-item>
                      </v-list-item-group>
                  </v-list> -->
                  
                  <v-select
                      v-model="shareDialogStore.data.recipient"
                      :items="nearbyUsers"
                      item-text="name"
                      item-value="uuid"
                      :rules="[v => !!v || 'A recipient is required']"
                      label="Nearby Users"
                      required
                  ></v-select>

                  <v-text-field
                      class="px-2"
                      label="URL"
                      :rules="[v => !!v || 'URL is required.']"
                      v-model="shareDialogStore.data.url"
                      required
                  ></v-text-field>

                  <v-card-actions>

                      <v-btn
                          color="red"
                          class="px-3"
                          @click="shareDialogStore.show = false"
                      >
                          Cancel
                      </v-btn>
                      
                      <v-spacer></v-spacer>
                      
                      <v-btn
                          color="blue"
                          class="px-3"
                          :disabled="!$store.state.shareDialog.valid"
                          @click="shareDialogStore.show = false; shareItem($store.state.shareDialog.data.uuid);"
                      >
                          Send
                      </v-btn>
                      
                  </v-card-actions>
                  
              </v-form>
          </v-card>
        </v-dialog>
    </v-app>
</template>

<script>

var vue_this;

function browserDevelopment() {
    if (typeof EventBridge !== 'undefined') {
        return false; // We are in the browser, probably for development purposes.
    } else {
        return true; // We are in Vircadia.
    }
}

if (!browserDevelopment()) {
    // eslint-disable-next-line
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

// import draggable from 'vuedraggable'
import itemiterator from './components/ItemIterator'

export default {
    name: 'App',
    components: {
        // draggable,
        itemiterator
    },
    data: () => ({
        folderList: [],
        recursiveFolderHoldingList: [],
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
            "displayDensity": {
                "size": 1,
                "labels": [
                    "List",
                    "Compact",
                    "Large",
                ],
            },
        },
        appVersion: "1.3",
        darkTheme: true,
        drawer: false,
        disabledProp: true,
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
        pushToItems: function(type, name, folder, url, uuid) {
            var uuidToUse;

            if (uuid != null) {
                uuidToUse = uuid;
            } else {
                uuidToUse = this.createUUID();
            }
            
            var itemToPush =             
            {
                "hasChildren": false,
                "type": type,
                "name": name,
                "url": url,
                "folder": folder,
                "uuid": uuidToUse,
            };
            
            this.items.push(itemToPush);
            
            if (folder !== null && folder !== "No Folder") {
                this.moveItem(uuidToUse, folder);
            }
        },
        pushFolderToItems: function(name) {
            var folderToPush =             
            {
                "hasChildren": true,
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
                    detectedItemType = "MODEL";
                    break;
                case ".gltf":
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
            }
            
            if (detectedItemType == null) {
                // This is not a known item...
                detectedItemType = "UNKNOWN";
            }
            
            return detectedItemType;
        },
        checkItemType: function(itemType) {
            var detectedItemType = null;
            itemType = itemType.toUpperCase();
            
            this.$store.state.supportedItemTypes.forEach(function (itemTypeInList) {
                if (itemTypeInList == itemType) {
                    detectedItemType = itemTypeInList;
                }
            });
            
            if (detectedItemType == null) {
                // This is not a known item type...
                detectedItemType = "UNKNOWN";
            }
            
            return detectedItemType;
        },
        createFolder: function(name) {
            this.pushFolderToItems(name);
        },
        editFolder: function(uuid) {
            var findFolder = this.searchForItem(uuid);
            
            if (findFolder) {
                findFolder.returnedItem.name = this.$store.state.editFolderDialog.data.name;
                
                if (this.$store.state.editFolderDialog.data.folder !== null && this.$store.state.editFolderDialog.data.folder !== "No Change") {
                    if (findFolder.returnedItem.folder !== this.$store.state.editFolderDialog.data.folder && this.$store.state.editFolderDialog.data.folder !== "No Folder") {
                        console.info("This folder?", this.$store.state.editFolderDialog.data.folder);
                        this.moveFolder(uuid, this.$store.state.editFolderDialog.data.folder);
                    } else if (this.$store.state.editFolderDialog.data.folder === "No Folder") {
                        console.info("This folder TOP?", this.$store.state.editFolderDialog.data.folder);
                        this.moveFolder(uuid, "top");
                    }
                }
            }
        },
        addItem: function(name, folder, url) {
            var detectedFileType = this.detectFileType(url);
            var itemType;
                        
            if (detectedFileType == null || detectedFileType[0] == null) {
                itemType = "unknown";
            } else {
                itemType = this.checkFileType(detectedFileType[0]);
            }

            this.pushToItems(itemType, name, folder, url, null);
            
            this.$store.commit('mutate', {
                property: 'addDialog.data.name', 
                with: null
            });
            
            this.$store.commit('mutate', {
                property: 'addDialog.data.folder', 
                with: null
            });
            
            this.$store.commit('mutate', {
                property: 'addDialog.data.url', 
                with: null
            });
        },
        detectFileType: function(url) {    
            // Attempt the pure regex route...
            var extensionRegex = /\.[0-9a-z]+$/i; // to detect the file type based on extension in the URL.
            var detectedFileType = url.match(extensionRegex);
            
            // If that fails, let's try the traditional URL route.
            if (detectedFileType == null || detectedFileType[0] == null) {
                var urlExtensionRegex = /\.[0-9a-z]+$/i;
                let urlToParse = new URL(url);

                // Attempt the URL converted regex route...
                detectedFileType = urlToParse.pathname.match(urlExtensionRegex);
            } else if (detectedFileType == null || detectedFileType[0] == null) { // Still not working?!
                // Your URL sucks!
                detectedFileType = null; // We got nothin'.
            }
            
            return detectedFileType;
        },
        removeItem: function(uuid) {
            var findItem = this.searchForItem(uuid);
            findItem.parentArray.splice(findItem.iteration, 1);
        },
        removeFolder: function(uuid) {
            var findFolder = this.searchForItem(uuid);
            findFolder.parentArray.splice(findFolder.iteration, 1);
        },
        editItem: function(uuid) {    
            var findItem = this.searchForItem(uuid);
            findItem.returnedItem.type = this.checkItemType(this.$store.state.editDialog.data.type);
            findItem.returnedItem.name = this.$store.state.editDialog.data.name;
            findItem.returnedItem.folder = this.$store.state.editDialog.data.folder;
            findItem.returnedItem.url = this.$store.state.editDialog.data.url;
            
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
        receivingItem: function(data) {
            if (this.$store.state.receiveDialog.show != true) { // Do not accept offers if the user is already receiving an offer.
            
                this.$store.commit('mutate', {
                    property: 'receiveDialog.data.user', 
                    with: data.data.user
                });
                
                this.$store.commit('mutate', {
                    property: 'receiveDialog.data.type', 
                    with: data.data.type
                });
                
                this.$store.commit('mutate', {
                    property: 'receiveDialog.data.name', 
                    with: data.data.name
                });
                
                this.$store.commit('mutate', {
                    property: 'receiveDialog.data.url', 
                    with: data.data.url
                });
                
                this.getFolderList("add");
                                
                this.$store.commit('mutate', {
                    property: 'receiveDialog.show', 
                    with: true
                });
            }
        },
        shareItem: function(uuid) {        
            var findItem = this.searchForItem(uuid);
            var typeToShare = findItem.returnedItem.type;
            var nameToShare = findItem.returnedItem.name;
            
            // alert("type" + typeToShare + "name" + nameToShare);
            this.sendAppMessage("share-item", {
                "type": typeToShare,
                "name": nameToShare,
                "url": this.$store.state.shareDialog.data.url,
                "recipient": this.$store.state.shareDialog.data.recipient,
            });
        },
        acceptItem: function() {
            this.pushToItems(
                this.checkItemType(this.$store.state.receiveDialog.data.type), 
                this.$store.state.receiveDialog.data.name,
                this.$store.state.receiveDialog.data.folder,
                this.$store.state.receiveDialog.data.url,
                null
            );
        },
        useItem: function(type, url) {
            this.sendAppMessage("use-item", { 
                "type": type, 
                "url": url 
            });
        },
        onDragStart: function() {
            console.info("Drag start.");
        },
        onDragUpdate: function() {
            console.info("Drag Update.");
        },
        onDragEnd: function() {
            console.info("Drag End.");
        },
        onDragChange: function(ev) {
            console.info("Drag Update.", ev);
        },
        sortTopInventory: function(level) {
            if (level == "top") {
                this.$store.commit('sortTopInventory');
            }
        },
        getFolderList: function(request) {
            var generateList;
            this.recursiveFolderHoldingList = []; // Clear that list before we do anything.
            
            if (request == "edit") {
                this.folderList = [
                    {
                        "name": "No Change",
                        "uuid": "No Change"
                    },
                    {
                        "name": "No Folder", 
                        "uuid": "No Folder"
                    },
                ];
                
                generateList = this.recursiveFolderPopulate(this.itemsStore, true, null);
                
            } else if (request == "add") {
                this.folderList = [
                    {
                        "name": "No Folder", 
                        "uuid": "No Folder"
                    },
                ];
                
                generateList = this.recursiveFolderPopulate(this.itemsStore, true, null);
                
            } else if (request == "editFolder") {
                this.folderList = [
                    {
                        "name": "No Change",
                        "uuid": "No Change"
                    },
                    {
                        "name": "No Folder", 
                        "uuid": "No Folder"
                    },
                ];
                
                generateList = this.recursiveFolderPopulate(this.itemsStore, true, this.$store.state.editFolderDialog.data.uuid);
            }
            
            if (generateList) {
                var combinedArray = this.folderList.concat(generateList);
                this.folderList = combinedArray;
            }
        },
        moveItem: function(uuid, folderUUID) {
            var findItem = this.searchForItem(uuid);
            
            if (folderUUID === "top") {
                // Remove the old item before placing down the copy, we already got the attributes that we had wanted.
                this.removeItem(uuid);
    
                this.pushToItems(
                    findItem.returnedItem.type, 
                    findItem.returnedItem.name, 
                    "No Folder", 
                    findItem.returnedItem.url, 
                    uuid
                );
                
            } else {
                
                var itemToPush = {
                    "hasChildren": false,
                    'type': null,
                    'name': null,
                    'folder': null,
                    'url': null,
                    'uuid': uuid,
                };

                itemToPush.type = findItem.returnedItem.type;
                itemToPush.name = findItem.returnedItem.name;
                itemToPush.url = findItem.returnedItem.url;
    
                // Get the folder UUID.
                for (var i = 0; i < this.folderList.length; i++) {
                    if (this.folderList[i].uuid === folderUUID) {
                        itemToPush.folder = this.folderList[i].name;
                    }
                }
                
                // Remove the old item before placing down the copy, we already got the attributes that we had wanted.
                this.removeItem(uuid);
    
                // Find that folder in our main items array.
                var findFolder = this.searchForItem(folderUUID);
                
                if (findFolder) {
                    findFolder.returnedItem.items.push(itemToPush);
                }
            }

        },
        moveFolder: function(uuid, parentFolderUUID) {
            var findFolder = this.searchForItem(uuid);
            var findParentFolder;
            
            if (parentFolderUUID !== "top") {
                findParentFolder = this.searchForItem(parentFolderUUID);
            }
            
            // this.removeFolder(uuid);
            
            this.$store.commit('moveFolder', {
                "uuid": uuid,
                "parentFolderUUID": parentFolderUUID,
                "findFolder": findFolder,
                "findParentFolder": findParentFolder
            });
            
        },
        searchForItem: function(uuid) {
            var foundItem = this.recursiveSingularSearch(uuid, this.itemsStore);
            
            if (foundItem) {
                return {
                    "returnedItem": foundItem.returnedItem,
                    "iteration": foundItem.iteration,
                    "parentArray": foundItem.parentArray,
                    "itemUUID": uuid,
                }
            }
        },
        recursiveSingularSearch: function(uuid, indexToSearch) {
            for (var i = 0; i < indexToSearch.length; i++) {
                if (indexToSearch[i].uuid == uuid) {
                    var foundItem = {
                        "returnedItem": indexToSearch[i],
                        "iteration": i,
                        "parentArray": indexToSearch,
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
        recursiveFolderPopulate: function(indexToSearch, firstIteration, avoidFolder) {
            console.info(JSON.stringify(this.folderList));
            for (var i = 0; i < indexToSearch.length; i++) {
                if (Object.prototype.hasOwnProperty.call(indexToSearch[i], "items") && indexToSearch[i].items.length > 0) {
                    // We want to avoid adding the folder itself and also any child folders it may have, putting a folder within its child folder will nuke it.
                    if (avoidFolder !== indexToSearch[i].uuid) {
                        this.recursiveFolderHoldingList.push({
                            "name": indexToSearch[i].name,
                            "uuid": indexToSearch[i].uuid,
                        });
                        
                        this.recursiveFolderPopulate(indexToSearch[i].items, false, avoidFolder);
                    }
                }
            }
            
            if (firstIteration === true) {
                return this.recursiveFolderHoldingList;
            }
        },
        sendInventory: function() {
            this.sendAppMessage("web-to-script-inventory", this.itemsStore );
        },
        receiveInventory: function(receivedInventory) {
            if (!receivedInventory) {
                this.itemsStore = [];
            } else {
                this.itemsStore = receivedInventory;
            }
        },
        sendSettings: function() {
            this.sendAppMessage("web-to-script-settings", this.$store.state.settings );
        },
        receiveSettings: function(receivedSettings) {
            if (!receivedSettings) {
                // Don't do anything, let the defaults stand. Otherwise, it will break the app.
            } else {
                this.settings = receivedSettings;
            }
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
                // eslint-disable-next-line
                EventBridge.emitWebEvent(JSON.stringify(JSONtoSend));
            } else {
                // alert(JSON.stringify(JSONtoSend));
            }
        },
    },
    computed: {
        itemsStore: {
            get() {
                return this.$store.state.items;
            },
            set(value) {
                this.$store.commit('mutate', {
                    property: 'items', 
                    with: value
                });
            },
        },
        addDialogStore: {
            get() {
                return this.$store.state.addDialog;
            },
            set(value) {
                this.$store.commit('mutate', {
                    property: 'addDialog', 
                    with: value
                });
            },
        },
        editDialogStore: {
            get() {
                return this.$store.state.editDialog;
            },
            set(value) {
                this.$store.commit('mutate', {
                    property: 'editDialog', 
                    with: value
                });
            },
        },
        editDialogShow: function() {
            return this.$store.state.editDialog.show;
        },
        editFolderDialogStore: {
            get() {
                return this.$store.state.editFolderDialog;
            },
            set(value) {
                this.$store.commit('mutate', {
                    property: 'editFolderDialog', 
                    with: value
                });
            },
        },
        editFolderDialogShow: function() {
            return this.$store.state.editFolderDialog.show;
        },
        createFolderDialogStore: {
            get() {
                return this.$store.state.createFolderDialog;
            },
            set(value) {
                this.$store.commit('mutate', {
                    property: 'createFolderDialog', 
                    with: value
                });
            },
        },
        receiveDialogStore: {
            get() {
                return this.$store.state.receiveDialog;
            },
            set(value) {
                this.$store.commit('mutate', {
                    property: 'receiveDialog', 
                    with: value
                });
            },
        },
        shareDialogStore: {
            get() {
                return this.$store.state.shareDialog;
            },
            set(value) {
                this.$store.commit('mutate', {
                    property: 'shareDialog', 
                    with: value
                });
                
                this.sendAppMessage('web-to-script-request-nearby-users', '')
            },
        },
        removeFolderDialogStore: {
            get() {
                return this.$store.state.removeFolderDialog;
            },
            set(value) {
                this.$store.commit('mutate', {
                    property: 'removeFolderDialog', 
                    with: value
                });
            },
        },
        removeDialogStore: {
            get() {
                return this.$store.state.removeDialog;
            },
            set(value) {
                this.$store.commit('mutate', {
                    property: 'removeDialog', 
                    with: value
                });
            },
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
        settings: {
            deep: true,
            handler: function(newVal) {
                this.$store.commit('mutate', {
                    property: 'settings', 
                    with: newVal
                });
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
        }
    }
};

</script>
