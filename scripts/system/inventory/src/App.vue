<!--
//
//  App.vue
//
//  Created by kasenvr@gmail.com on 7 Apr 2020
//  Copyright 2020 Vircadia Contributors
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
            
            <v-btn medium color="primary" fab @click="sortInventory('top')">
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

                    <v-list-item @click="addDialog.show = true; getFolderList('add');">
                        <v-list-item-icon>
                            <v-icon>mdi-plus</v-icon>
                        </v-list-item-icon>
                        <v-list-item-title>Add Item</v-list-item-title>
                    </v-list-item>

                    <v-list-item @click="createFolderDialog.show = true">
                        <v-list-item-icon>
                            <v-icon>mdi-folder-plus</v-icon>
                        </v-list-item-icon>
                        <v-list-item-title>Create Folder</v-list-item-title>
                    </v-list-item>
                                    
                    <p class="appVersion font-weight-light">Version {{settings.appVersion}}</p>

                </v-list-item-group>
            </v-list>
        </v-navigation-drawer>

        <v-content>
            <v-container fluid>
                <v-data-iterator
                    :items="items"
                    hide-default-footer
                >
                    <template>
                        <v-col
                            cols="12"
                            sm="6"
                            md="4"
                            lg="3"
                            class="py-1 column-item"
                        >
                            <draggable :group="options" :list="items" handle=".handle">                                
                                <v-item-group
                                    v-for="item in items"
                                    v-bind:key="item.uuid"
                                >
                                    <v-list-item 
                                        one-line 
                                        v-if="!item.isFolder"
                                        class="mx-auto draggable-card"
                                        max-width="344"
                                        outlined
                                    >
                                          <div class="handle pa-2">
                                              <v-icon color="orange darken-2">mdi-blur-linear</v-icon>
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
                                                  @click="useItem(item.type, item.url)"
                                              >
                                                  <v-list-item-title>Use</v-list-item-title>
                                                  <v-list-item-action>
                                                      <v-icon>mdi-play</v-icon>
                                                  </v-list-item-action>
                                              </v-list-item>
                                              <v-list-item
                                                  @click="
                                                      editDialog.show = true; 
                                                      editDialog.uuid = item.uuid;
                                                      editDialog.data.type = item.type.toUpperCase();
                                                      editDialog.data.folder = null;
                                                      editDialog.data.name = item.name;
                                                      editDialog.data.url = item.url;
                                                      getFolderList('edit');
                                                  "
                                              >
                                                  <v-list-item-title>Edit</v-list-item-title>
                                                  <v-list-item-action>
                                                      <v-icon>mdi-pencil</v-icon>
                                                  </v-list-item-action>
                                              </v-list-item>
                                              <v-list-item
                                                  @click="shareDialog.show = true; shareDialog.data.url = item.url; shareDialog.data.uuid = item.uuid; sendAppMessage('web-to-script-request-nearby-users', '')"
                                              >
                                                  <v-list-item-title>Share</v-list-item-title>
                                                  <v-list-item-action>
                                                      <v-icon>mdi-share</v-icon>
                                                  </v-list-item-action>
                                              </v-list-item>
                                              <v-list-item
                                                  @click="removeDialog.show = true; removeDialog.uuid = item.uuid;"
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
                                        v-if="item.isFolder"
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
                                        <v-btn medium color="primary" class="mx-1 my-1 folder-button"
                                            @click="
                                                editFolderDialog.show = true; 
                                                editFolderDialog.uuid = item.uuid;
                                                editFolderDialog.data.name = item.name;
                                            "
                                        >
                                            Edit Folder
                                        </v-btn>
                                        <v-btn medium color="red" class="mx-1 my-1 folder-button"
                                            @click="removeFolderDialog.show = true; removeFolderDialog.uuid = item.uuid;"
                                        >
                                            Delete Folder
                                        </v-btn>
                                        <v-btn medium color="purple" class="mx-1 my-1 folder-button"
                                            @click="sortFolder(item.uuid);"
                                        >
                                            Sort Folder
                                        </v-btn>
                                        <v-col
                                            cols="12"
                                            sm="6"
                                            md="4"
                                            lg="3"
                                            class="py-1 column-item"
                                        >
                                            <draggable 
                                                :list="item.items"
                                                :group="options"
                                            >
                                                <v-item-group
                                                    v-for="item in item.items"
                                                    v-bind:key="item.uuid"
                                                >
                                                    <v-list-item 
                                                        one-line
                                                        class="mx-auto draggable-card"
                                                        outlined
                                                    >
                                                        <div class="handle pa-2">
                                                            <v-icon color="orange darken-2">mdi-blur-linear</v-icon>
                                                        </div>
                                                        <v-list-item-content class="pb-1 pt-2">
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
                                                                    @click="useItem(item.type, item.url)"
                                                                >
                                                                    <v-list-item-title>Use</v-list-item-title>
                                                                    <v-list-item-action>
                                                                        <v-icon>mdi-play</v-icon>
                                                                    </v-list-item-action>
                                                                </v-list-item>
                                                                
                                                                <v-list-item
                                                                    @click="
                                                                        editDialog.show = true; 
                                                                        editDialog.uuid = item.uuid;
                                                                        editDialog.data.type = item.type.toUpperCase();
                                                                        editDialog.data.folder = null;
                                                                        editDialog.data.name = item.name;
                                                                        editDialog.data.url = item.url;
                                                                        getFolderList('edit');
                                                                    "
                                                                >
                                                                    <v-list-item-title>Edit</v-list-item-title>
                                                                    <v-list-item-action>
                                                                        <v-icon>mdi-pencil</v-icon>
                                                                    </v-list-item-action>
                                                                </v-list-item>
                                                                
                                                                <v-list-item
                                                                    @click="shareDialog.show = true; shareDialog.data.url = item.url; shareDialog.data.uuid = item.uuid; sendAppMessage('web-to-script-request-nearby-users', '')"
                                                                >
                                                                    <v-list-item-title>Share</v-list-item-title>
                                                                    <v-list-item-action>
                                                                        <v-icon>mdi-share</v-icon>
                                                                    </v-list-item-action>
                                                                </v-list-item>
                                                                
                                                                <v-list-item
                                                                    @click="removeDialog.show = true; removeDialog.uuid = item.uuid;"
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
                                                </v-item-group>
                                            </draggable>
                                        </v-col>
                                    </v-list-group>
                                </v-item-group>
                            </draggable>
                        </v-col>
                    </template>
                </v-data-iterator>
            </v-container>
        </v-content>

        <v-dialog
          v-model="removeDialog.show"
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
                      @click="removeDialog.show = false"
                  >
                      No
                  </v-btn>
                  
                  <v-spacer></v-spacer>
                  
                  <v-btn
                      color="red"
                      class="px-3"                    
                      @click="removeDialog.show = false; removeItem(removeDialog.uuid);"
                  >
                      Yes
                  </v-btn>
                  
              </v-card-actions>
              
          </v-card>
        </v-dialog>
        
        <v-dialog
          v-model="removeFolderDialog.show"
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
                      @click="removeFolderDialog.show = false"
                  >
                      No
                  </v-btn>
                  
                  <v-spacer></v-spacer>
                  
                  <v-btn
                      color="red"
                      class="px-3"                    
                      @click="removeFolderDialog.show = false; removeFolder(removeFolderDialog.uuid);"
                  >
                      Yes
                  </v-btn>
                  
              </v-card-actions>
              
          </v-card>
        </v-dialog>

        <v-dialog
          v-model="editDialog.show"
          max-width="380"
        >
          <v-card>
              <v-card-title class="headline">Edit Item</v-card-title>
              
              <v-form
                  ref="editForm"
                  v-model="editDialog.valid"
                  :lazy-validation="false"
              >
                    
                    <v-select
                        :items="supportedItemTypes"
                        class="my-2"
                        v-model="editDialog.data.type"
                        :rules="[v => !!v || 'Type is required.']"
                        label="Item Type"
                        outlined
                    ></v-select>

                    <v-text-field
                        class="px-2"
                        label="Name"
                        v-model="editDialog.data.name"
                        :rules="[v => !!v || 'Name is required.']"
                        required
                    ></v-text-field>

                    <v-select
                        :items="folderList"
                        item-text="name"
                        item-value="uuid"
                        class="my-2"
                        v-model="editDialog.data.folder"
                        label="Folder"
                        outlined
                    ></v-select>

                    <v-text-field
                        class="px-2"
                        label="URL"
                        v-model="editDialog.data.url"
                        :rules="[v => !!v || 'URL is required.']"
                        required
                    ></v-text-field>

                  <v-card-actions>

                      <v-btn
                          color="red"
                          class="px-3"
                          @click="editDialog.show = false"
                      >
                          Cancel
                      </v-btn>
                      
                      <v-spacer></v-spacer>
                      
                      <v-btn
                          color="blue"
                          class="px-3"       
                          :disabled="!editDialog.valid"             
                          @click="editDialog.show = false; editItem(editDialog.uuid);"
                      >
                          Done
                      </v-btn>
                  
                  </v-card-actions>
                  
              </v-form>

          </v-card>
        </v-dialog>
        
        <v-dialog
          v-model="editFolderDialog.show"
          max-width="380"
        >
          <v-card>
              <v-card-title class="headline">Edit Folder</v-card-title>
              
              <v-form
                  ref="editFolderForm"
                  v-model="editFolderDialog.valid"
                  :lazy-validation="false"
              >

                  <v-text-field
                      class="px-2"
                      label="Name"
                      v-model="editFolderDialog.data.name"
                      :rules="[v => !!v || 'Name is required.']"
                      required
                  ></v-text-field>

                  <v-card-actions>

                      <v-btn
                          color="red"
                          class="px-3"
                          @click="editFolderDialog.show = false"
                      >
                          Cancel
                      </v-btn>
                      
                      <v-spacer></v-spacer>
                      
                      <v-btn
                          color="blue"
                          class="px-3"       
                          :disabled="!editFolderDialog.valid"             
                          @click="editFolderDialog.show = false; editFolder(editFolderDialog.uuid);"
                      >
                          Done
                      </v-btn>
                  
                  </v-card-actions>
                  
              </v-form>

          </v-card>
        </v-dialog>

        <v-dialog
          v-model="createFolderDialog.show"
          max-width="380"
        >
          <v-card>
              <v-card-title class="headline">Create Folder</v-card-title>
              
              <v-card-text>
                  Enter the name of the folder.
              </v-card-text>
              
              <v-form
                  ref="createFolderForm"
                  v-model="createFolderDialog.valid"
                  :lazy-validation="false"
              >

                  <v-text-field
                      class="px-2"
                      label="Name"
                      v-model="createFolderDialog.data.name"
                      :rules="[v => !!v || 'Name is required.']"
                      required
                  ></v-text-field>

                  <v-card-actions>

                      <v-btn
                          color="red"
                          class="px-3"
                          @click="createFolderDialog.show = false"
                      >
                          Cancel
                      </v-btn>
                      
                      <v-spacer></v-spacer>
                      
                      <v-btn
                          color="blue"
                          class="px-3"
                          :disabled="!createFolderDialog.valid"
                          @click="createFolderDialog.show = false; createFolder(createFolderDialog.data.name)"
                      >
                          Create
                      </v-btn>
                      
                  </v-card-actions>
              
              </v-form>
          </v-card>
        </v-dialog>

        <v-dialog
          v-model="addDialog.show"
          max-width="380"
        >
          <v-card>
              <v-card-title class="headline">Add Item</v-card-title>
             
              
              <v-form
                  ref="addForm"
                  v-model="addDialog.valid"
                  :lazy-validation="false"
              >
              
                  <v-card-text>
                      Enter the name of the item.
                  </v-card-text>

                  <v-text-field
                      class="px-2"
                      label="Name"
                      v-model="addDialog.data.name"
                      :rules="[v => !!v || 'Name is required.']"
                      required
                  ></v-text-field>
                  
                  <v-card-text>
                      Select a folder (optional).
                  </v-card-text>
                  
                  <v-select
                      class="my-2"
                      :items="folderList"
                      v-model="addDialog.data.folder"
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
                      v-model="addDialog.data.url"
                      :rules="[v => !!v || 'URL is required.']"
                      required
                  ></v-text-field>

                  <v-card-actions>

                      <v-btn
                          color="red"
                          class="px-3"
                          @click="addDialog.show = false"
                      >
                          Cancel
                      </v-btn>
                      
                      <v-spacer></v-spacer>
                      
                      <v-btn
                          color="blue"
                          class="px-3"
                          :disabled="!addDialog.valid"
                          @click="addDialog.show = false; addItem(addDialog.data.name, addDialog.data.folder, addDialog.data.url)"
                      >
                          Add
                      </v-btn>
                      
                  </v-card-actions>
              
              </v-form>
          </v-card>
        </v-dialog>

        <v-dialog
          v-model="receiveDialog.show"
          max-width="380"
          persistent
        >
          <v-card>
              <v-card-title class="headline">Receiving Item</v-card-title>

              <v-card-text>
                  {{receiveDialog.data.user}} is sending you an item.
              </v-card-text>
              
              <v-form
                  ref="receiveForm"
                  v-model="receiveDialog.valid"
                  :lazy-validation="false"
              >
              
                  <v-text-field
                      class="px-2"
                      label="Type"
                      :rules="[v => !!v || 'Type is required.']"
                      v-model="receiveDialog.data.type"
                      required
                  ></v-text-field>
                  
                  <v-text-field
                      class="px-2"
                      label="Name"
                      :rules="[v => !!v || 'Name is required.']"
                      v-model="receiveDialog.data.name"
                      required
                  ></v-text-field>
                  
                  <v-card-text>
                      Select a folder (optional).
                  </v-card-text>
                  
                  <v-select
                      class="my-2"
                      :items="folderList"
                      v-model="receiveDialog.data.folder"
                      label="Folder"
                      outlined
                      item-text="name"
                      item-value="uuid"
                  ></v-select>

                  <v-text-field
                      class="px-2"
                      label="URL"
                      :rules="[v => !!v || 'URL is required.']"
                      v-model="receiveDialog.data.url"
                      required
                  ></v-text-field>

                  <v-card-actions>

                      <v-btn
                          color="red"
                          class="px-3"
                          @click="receiveDialog.show = false"
                      >
                          Reject
                      </v-btn>
                      
                      <v-spacer></v-spacer>
                      
                      <v-btn
                          color="blue"
                          class="px-3"
                          :disabled="!receiveDialog.valid"
                          @click="receiveDialog.show = false; acceptItem();"
                      >
                          Accept
                      </v-btn>
                      
                  </v-card-actions>
                  
              </v-form>
          </v-card>
        </v-dialog>

        <v-dialog
          v-model="shareDialog.show"
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
                  v-model="shareDialog.valid"
                  :lazy-validation="false"
                  class="px-2"
              >
              
                  <!-- <v-list>
                      <v-list-item-group v-model="shareDialog.data.recipient" color="primary">
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
                      v-model="shareDialog.data.recipient"
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
                      v-model="shareDialog.data.url"
                      required
                  ></v-text-field>

                  <v-card-actions>

                      <v-btn
                          color="red"
                          class="px-3"
                          @click="shareDialog.show = false"
                      >
                          Cancel
                      </v-btn>
                      
                      <v-spacer></v-spacer>
                      
                      <v-btn
                          color="blue"
                          class="px-3"
                          :disabled="!shareDialog.valid"
                          @click="shareDialog.show = false; shareItem(shareDialog.data.uuid);"
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

import draggable from 'vuedraggable'

export default {
    name: 'App',
    components: {
        draggable,
    },
    data: () => ({
        items: [
            {
                "type": "script",
                "name": "VRGrabScale",
                "url": "https://gooawefaweawfgle.com/vr.js",
                "folder": "No Folder",
                "uuid": "54254354353",
            },
            {
                "isFolder": true,
                "name": "Test Folder",
                "items": [
                    {
                        "type": "script",
                        "name": "TESTFOLDERSCRIPT",
                        "url": "https://googfdafsgaergale.com/vr.js",
                        "folder": "No Folder",
                        "uuid": "54hgfhgf25fdfadf4354353",
                    },
                    {
                        "type": "script",
                        "name": "FOLDERSCRIPT2",
                        "url": "https://googfdafsgaergale.com/vr.js",
                        "folder": "No Folder",
                        "uuid": "54hgfhgf25ffdafddfadf4354353",
                    },
                ],
                "uuid": "54354363wgsegs45ujs",
            },
            {
                "type": "script",
                "name": "VRGrabScale",
                "url": "https://googfdafsgaergale.com/vr.js",
                "folder": "No Folder",
                "uuid": "54hgfhgf254354353",
            },
            {
                "type": "script",
                "name": "TEST",
                "url": "https://gooadfdagle.com/vr.js",
                "folder": "No Folder",
                "uuid": "542rfwat4t5fsddf4354353",
            },
            {
                "type": "json",
                "name": "TESTJSON",
                "url": "https://gooadfdagle.com/vr.json",
                "folder": "No Folder",
                "uuid": "542rfwat4t54354353",
            },
            {
                "type": "script",
                "name": "TESTLONGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG",
                "url": "https://googfdaffle.com/vrLONGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG.js",
                "folder": "No Folder",
                "uuid": "5425ggsrg45354353",
            },
            {
                "type": "avatar",
                "name": "AVI",
                "url": "https://googlfadfe.com/vr.fst",
                "folder": "No Folder",
                "uuid": "542gregg45s3g4354353",
            },
            {
                "type": "avatar",
                "name": "AVI",
                "url": "https://googlefdaf.com/vr.fst",
                "folder": "No Folder",
                "uuid": "5420798-087-54354353",
            },
            {
                "type": "model",
                "name": "3D MODEL",
                "url": "https://googlee.com/vr.fbx",
                "folder": "No Folder",
                "uuid": "54254354980-7667jt353",
            },
            {
                "type": "place",
                "name": "PLACE DOMAIN",
                "url": "https://googleee.com/vr.fbx",
                "folder": "No Folder",
                "uuid": "542543sg45s4gg54353",
            },
        ],
        iconType: {
            "SCRIPT": {
                "icon": "mdi-code-tags",
                "color": "red",
            },
            "MODEL": {
                "icon": "mdi-video-3d",
                "color": "green",
            },
            "AVATAR": {
                "icon": "mdi-account-convert",
                "color": "purple",
            },
            "PLACE": {
                "icon": "mdi-earth",
                "color": "#0097A7", // cyan darken-2
            },
            "JSON": {
                "icon": "mdi-inbox-multiple",
                "color": "#37474F", // blue-grey darken-3
            },
            "UNKNOWN": {
                "icon": "mdi-help",
                "color": "grey",
            }
        },
        supportedItemTypes: [
            "SCRIPT",
            "MODEL",
            "AVATAR",
            "PLACE",
            "JSON",
            "UNKNOWN",
        ],
        removeDialog: {
            show: false,
            uuid: null,
        },
        removeFolderDialog: {
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
                "folder": null,
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
                "folder": null,
            },
        },
        editFolderDialog: {
            show: false,
            valid: false,
            uuid: null, //
            data: {
                "name": null,
            },
        },
        receiveDialog: {
            show: false,
            valid: false,
            data: {
                "user": null,
                "name": null,
                "folder": null,
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
        folderList: [],
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
            appVersion: "1.2.1",
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
        pushToItems: function(type, name, folder, url, uuid) {
            var uuidToUse;

            if (uuid != null) {
                uuidToUse = uuid;
            } else {
                uuidToUse = this.createUUID();
            }
            
            var itemToPush =             
            {
                "type": type,
                "name": name,
                "url": url,
                "folder": folder,
                "uuid": uuidToUse,
            };
            
            this.items.push(itemToPush);
            
            if (folder !== null && folder !== "No Folder") {
                this.moveItemToFolder(uuidToUse, folder);
            }
        },
        pushFolderToItems: function(name) {
            var folderToPush =             
            {
                "isFolder": true,
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
            
            this.supportedItemTypes.forEach(function(itemTypeInList) {
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
            for (var i = 0; i < this.items.length; i++) {
                if (this.items[i].uuid == uuid) {
                    this.items[i].name = this.editFolderDialog.data.name;
                    
                    return;
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
            
            this.addDialog.data.name = null;
            this.addDialog.data.folder = null;
            this.addDialog.data.url = null;
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
            for (var i = 0; i < this.items.length; i++) {
                if (this.items[i].uuid == uuid) {
                    this.items.splice(i, 1);
                    
                    return;
                }
            }
        },
        editItem: function(uuid) {    
            var findItem = this.searchForItem(uuid);
                        
            findItem.returnedItem.type = this.checkItemType(this.editDialog.data.type);
            findItem.returnedItem.name = this.editDialog.data.name;
            findItem.returnedItem.folder = this.editDialog.data.folder;
            findItem.returnedItem.url = this.editDialog.data.url;
            
            var folderName;
            
            for (var i = 0; i < this.folderList.length; i++) {
                if (this.folderList[i].name === findItem.returnedItem.folder) {
                    folderName = this.folderList[i].name;
                }
            }
            
            if (this.editDialog.data.folder !== null) {
                if (folderName !== this.editDialog.data.folder && this.editDialog.data.folder !== "No Folder") {
                    this.moveItemToFolder(uuid, this.editDialog.data.folder);
                } else if (folderName === "No Folder") {
                    this.moveItemToTop(uuid);
                }
            }

        },
        receivingItem: function(data) {
            if (this.receiveDialog.show != true) { // Do not accept offers if the user is already receiving an offer.
                this.receiveDialog.data.user = data.data.user;
                this.receiveDialog.data.type = data.data.type;
                this.receiveDialog.data.name = data.data.name;
                this.receiveDialog.data.url = data.data.url;
                
                this.getFolderList("add");
                
                this.receiveDialog.show = true;
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
                "url": this.shareDialog.data.url,
                "recipient": this.shareDialog.data.recipient,
            });
        },
        acceptItem: function() {
            this.pushToItems(
                this.checkItemType(this.receiveDialog.data.type), 
                this.receiveDialog.data.name,
                this.receiveDialog.data.folder,
                this.receiveDialog.data.url,
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
        sortInventory: function(level) {
            if (level == "top") {
                this.items.sort(function(a, b) {
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
        },
        sortFolder: function(uuid) {
            for (var i = 0; i < this.items.length; i++) {
                if (this.items[i].uuid == uuid) {
                    this.items[i].items.sort(function(a, b) {
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
            }
        },
        getFolderList: function(request) {
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
            } else if (request == "add") {
                this.folderList = [
                    {
                        "name": "No Folder", 
                        "uuid": "No Folder"
                    },
                ];
            }
                        
            for (var i = 0; i < this.items.length; i++) {
                if (Object.prototype.hasOwnProperty.call(this.items[i], "isFolder")) {
                    if (this.items[i].isFolder === true) {
                        this.folderList.push({
                            "name": this.items[i].name,
                            "uuid": this.items[i].uuid,
                        });
                    }
                }
            }
        },
        moveItemToFolder: function(uuid, folderUUID) {
            // This function is used to take an item one level deep, do not use it for any other purposes and check beforehand if you need to do this.
            var itemToPush = {
                'type': null,
                'name': null,
                'folder': null,
                'url': null,
                'uuid': uuid,
            };
            
            var findItem = this.searchForItem(uuid);
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
            for (var folder = 0; folder < this.items.length; folder++) { 
                if (this.items[folder].uuid === folderUUID && this.items[folder].isFolder === true) {
                    this.items[folder].items.push(itemToPush);
                }
            }
            
        },
        moveItemToTop: function(uuid) {
            var findItem = this.searchForItem(uuid);
            
            // Remove the old item before placing down the copy, we already got the attributes that we had wanted.
            this.removeItem(uuid);

            this.pushToItems(
                findItem.returnedItem.type, 
                findItem.returnedItem.name, 
                "No Folder", 
                findItem.returnedItem.url, 
                uuid
            );
        },
        searchForItem: function(uuid) {
            var itemToReturn = {
                "returnedItem": null,
                "iteration": null,
                "parentArray": null,
                "itemUUID": uuid,
            }
            
            for (var i = 0; i < this.items.length; i++) {
                if (this.items[i].uuid == uuid) {
                    itemToReturn.returnedItem = this.items[i];
                    itemToReturn.iteration = i;
                    itemToReturn.parentArray = this.items;
                    return itemToReturn;
                } else if (Object.prototype.hasOwnProperty.call(this.items[i], "items")) {
                    for (var di = 0; di < this.items[i].items.length; di++) { // DI means deep iteration
                        if (this.items[i].items[di].uuid == uuid) { 
                            itemToReturn.returnedItem = this.items[i].items[di];   
                            itemToReturn.iteration = di;
                            itemToReturn.parentArray = this.items[i].items;
                            return itemToReturn;    
                        }
                    }
                }
            }
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
        getIcon: function(itemType) {
            itemType = itemType.toUpperCase();
            return this.iconType[itemType].icon;
        },
        getIconColor: function(itemType) {
            itemType = itemType.toUpperCase();
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
                // eslint-disable-next-line
                EventBridge.emitWebEvent(JSON.stringify(JSONtoSend));
            } else {
                // alert(JSON.stringify(JSONtoSend));
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
        options : function (){
            return { 
                name: 'column-item',
                pull: true, 
                put: true 
            }
        }
    }
};

</script>
