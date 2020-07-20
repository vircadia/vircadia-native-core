<!--
//  ItemIterator.vue
//
//  Created by Kalila L. on 13 April 2020.
//  Copyright 2020 Vircadia and contributors..
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <draggable :group="options" :list="itemsForIterator" handle=".handle">           
        <v-item-group
            v-for="item in itemsForIterator"
            v-bind:key="item.uuid"
        >
            <v-list-item 
                one-line 
                v-if="!item.items"
                class="mx-auto draggable-card"
                max-width="344"
                outlined
            >
                  <div class="handle pa-2">
                      <!-- <v-icon color="orange darken-2">mdi-blur-linear</v-icon> -->
                      <!-- <v-icon color="orange darken-2">mdi-package-variant</v-icon> -->
                      <v-icon color="orange darken-2">mdi-square-medium-outline</v-icon>
                  </div>
                  <v-list-item-content 
                      class="pb-1 pt-2 pl-4" 
                  >
                      <div v-show="settings.displayDensity.size > 0" class="overline" style="font-size: 0.825rem !important;">{{item.type}}</div>
                      <v-list-item-title class="subtitle-1 mb-1">{{item.name}}</v-list-item-title>
                      <v-list-item-subtitle v-show="settings.displayDensity.size == 2">{{item.url}}</v-list-item-subtitle>
                  </v-list-item-content>
                  
                  <v-menu bottom left>
                      <template v-slot:activator="{ on }">
                          <!-- settings.displayDensity.size >= 1 -->
                          <v-btn 
                              :style="{backgroundColor: (getIconColor(item.type)) }"
                              v-show="settings.displayDensity.size >= 1"
                              medium 
                              fab 
                              dark
                              v-on="on"
                          >
                              <v-icon>{{getIcon(item.type)}}</v-icon>
                          </v-btn>
                          <!-- settings.displayDensity.size < 1 -->
                          <v-btn 
                              :style="{backgroundColor: (getIconColor(item.type)) }"
                              v-show="settings.displayDensity.size < 1"
                              small
                              fab
                              dark
                              v-on="on"
                          >
                              <v-icon>{{getIcon(item.type)}}</v-icon>
                          </v-btn>
                      </template>

                      <v-list color="grey darken-3">
                          <v-list-item
                              @click="sendEvent('use-item', { 'type': item.type, 'url': item.url })"
                          >
                              <v-list-item-title>Use</v-list-item-title>
                              <v-list-item-action>
                                  <v-icon>mdi-play</v-icon>
                              </v-list-item-action>
                          </v-list-item>
                          <v-list-item
                              @click="
                                  editDialogStore.show = true; 
                                  editDialogStore.uuid = item.uuid;
                                  editDialogStore.data.type = item.type.toUpperCase();
                                  editDialogStore.data.folder = null;
                                  editDialogStore.data.name = item.name;
                                  editDialogStore.data.url = item.url;
                              "
                          >
                              <v-list-item-title>Edit</v-list-item-title>
                              <v-list-item-action>
                                  <v-icon>mdi-pencil</v-icon>
                              </v-list-item-action>
                          </v-list-item>
                          <v-list-item
                              @click="
                                shareDialogStore.show = true; 
                                shareDialogStore.data.url = item.url; 
                                shareDialogStore.data.uuid = item.uuid; 
                              "
                          >
                              <v-list-item-title>Share</v-list-item-title>
                              <v-list-item-action>
                                  <v-icon>mdi-share</v-icon>
                              </v-list-item-action>
                          </v-list-item>
                          <v-list-item
                              @click="
                                removeDialogStore.show = true; 
                                removeDialogStore.uuid = item.uuid;
                              "
                              color="red darken-1"
                          >
                              <v-list-item-title>Remove</v-list-item-title>
                              <v-list-item-action>
                                  <v-icon>mdi-minus</v-icon>
                              </v-list-item-action>
                          </v-list-item>
                      </v-list>
                  </v-menu>
                  
              </v-list-item>

        
            <!-- The Folder List Item -->
            <v-list-group
                v-else
                class="top-level-folder"
            >
            <!-- prepend-icon="mdi-blur-linear" put this in the list group, no idea how to make it a handle yet though... -->
                <template v-slot:activator>
                    <v-list-item 
                        one-line 
                        class="mx-auto"
                        max-width="344"
                        outlined
                    >
                        <v-icon class="folder-icon" color="teal">mdi-folder-settings</v-icon>
                        {{item.name}}
                    </v-list-item>
                </template>
                <div class="text-center my-2">
                    <v-btn medium tile color="purple" class="mx-1 folder-button"
                        @click="
                            editFolderDialogStore.data.uuid = item.uuid;
                            editFolderDialogStore.data.name = item.name;
                            editFolderDialogStore.show = true; 
                        "
                    >
                        <v-icon>mdi-pencil</v-icon>
                    </v-btn>
                    <v-btn medium tile color="red" class="mx-1 folder-button"
                        @click="
                            removeFolderDialogStore.show = true; 
                            removeFolderDialogStore.uuid = item.uuid;
                        "
                    >
                        <v-icon>mdi-minus</v-icon>
                    </v-btn>
                    
                    <v-menu bottom left>
                        <template v-slot:activator="{ on }">
                            <v-btn 
                                medium tile 
                                color="blue" 
                                class="mx-1 folder-button"
                                v-on="on"
                            >
                                <v-icon>mdi-sort</v-icon>
                            </v-btn>
                        </template>
        
                        <v-list color="grey darken-3">
                            <v-list-item
                                @click="sortFolder(item.uuid, 'az');"
                            >
                                <v-list-item-title>A-Z</v-list-item-title>
                                <v-list-item-action>
                                    <v-icon large>mdi-sort-alphabetical-ascending</v-icon>
                                </v-list-item-action>
                            </v-list-item>
                            <v-list-item
                                @click="sortFolder(item.uuid, 'za');"
                            >
                                <v-list-item-title>Z-A</v-list-item-title>
                                <v-list-item-action>
                                    <v-icon large>mdi-sort-alphabetical-descending</v-icon>
                                </v-list-item-action>
                            </v-list-item>
                        </v-list>
                    </v-menu>
                </div>
                
                <v-container fluid>
                    <itemiterator v-bind:key="item.uuid" :itemsForIterator="item.items"></itemiterator>
                </v-container>
                
            </v-list-group>
        </v-item-group>
    </draggable>
</template>


<script>

import { EventBus } from '../plugins/event-bus.js';
import draggable from 'vuedraggable';

export default {
    name: 'itemiterator',
    components: {
        draggable
    },
    props: ['itemsForIterator'],
    data: () => ({
        settings: {}
    }),
    created: function () {
        this.settings = this.$store.state.settings;
    },
    computed: {
        options : function (){
            return { 
                name: 'column-item',
                pull: true, 
                put: true 
            }
        },
        settingsChanged() {
            return this.$store.state.settings;
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
            }
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
            }
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
            }
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
            }
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
            }
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
            }
        }
    },
    watch: {
        settingsChanged (newVal, oldVal) {
            console.info ("Settings previous value:", oldVal);
            if (newVal) {
                this.settings = newVal;
            }
        }
    },
    methods: {
        sendEvent: function(command, data) {
            EventBus.$emit(command, data);
        },
        getIcon: function(itemType) {
            itemType = itemType.toUpperCase();
            var returnedItemIcon;
            
            if (this.$store.state.iconType[itemType]) {
                returnedItemIcon = this.$store.state.iconType[itemType].icon;
            } else {
                returnedItemIcon = this.$store.state.iconType.UNKNOWN.icon;
            }
            
            return returnedItemIcon;
        },
        getIconColor: function(itemType) {
            itemType = itemType.toUpperCase();
            var returnedItemIconColor;
            
            if (this.$store.state.iconType[itemType]) {
                returnedItemIconColor = this.$store.state.iconType[itemType].color;
            } else {
                returnedItemIconColor = this.$store.state.iconType.UNKNOWN.color;
            }
            
            return returnedItemIconColor;
        },
        sortFolder: function(uuid, sort) {
            var findFolder = this.searchForItem(uuid);
            
            if (findFolder) {
                if (sort === "az") {
                    findFolder.returnedItem.items.sort(function(a, b) {
                        var nameA = a.name.toUpperCase(); // ignore upper and lowercase
                        var nameB = b.name.toUpperCase(); // ignore upper and lowercase
                        if (nameA < nameB) {
                            return -1;
                        }
                        if (nameA > nameB) {
                            return 1;
                        }
        
                        // names must be equal
                        return 0;
                    });
                }
                if (sort === "za") {
                    findFolder.returnedItem.items.sort(function(a, b) {
                        var nameA = a.name.toUpperCase(); // ignore upper and lowercase
                        var nameB = b.name.toUpperCase(); // ignore upper and lowercase
                        if (nameA > nameB) {
                            return -1;
                        }
                        if (nameA < nameB) {
                            return 1;
                        }
        
                        // names must be equal
                        return 0;
                    });
                }
            }
        },
        searchForItem: function(uuid) {
            var foundItem = this.recursiveSingularSearch(uuid, this.itemsForIterator);
            
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
                    return this.recursiveSingularSearch(uuid, indexToSearch[i].items);
                }
            }
        },
    }
};
</script>
