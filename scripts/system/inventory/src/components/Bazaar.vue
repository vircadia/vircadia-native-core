<!--
//  Bazaar.vue
//
//  Created by Kalila L. on 10 November 2020.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <v-container fluid>
        <v-col>
            <v-data-iterator
                :items="bazaarIteratorData"
                :items-per-page.sync="bazaarIteratorItemsPerPage"
                :page="bazaarIteratorPage"
                :search="$store.state.searchBox"
            >
                <template v-slot:header>
                    <v-toolbar
                        class="mb-2"
                        dark
                        flat
                    >
                        <v-toolbar-title>
                            <v-breadcrumbs :items="bazaarStore.breadcrumbs"></v-breadcrumbs>
                        </v-toolbar-title>
                    </v-toolbar>
                </template>

                <template v-slot:default="props">
                    <v-row>
                        <v-col
                            v-for="item in props.items"
                            :key="item.main"
                            cols="12"
                            sm="6"
                            md="4"
                            lg="3"
                        >
                            <v-card>
                                <v-list-item>
                                    <v-list-item-avatar>
                                        <v-img 
                                            :src="item.icon"
                                            alt="Preview"
                                        ></v-img>
                                    </v-list-item-avatar>

                                    <v-list-item-content
                                        @click="showItemPage = true"
                                    >
                                        <v-list-item-title v-html="item.name"></v-list-item-title>
                                        <v-list-item-subtitle v-html="item.description"></v-list-item-subtitle>
                                    </v-list-item-content>
                                    
                                    <v-list-item-action>
                                        <v-btn
                                            class="mx-2"
                                            fab
                                            color="primary"
                                            small
                                            @click="addItemToInventory(item)"
                                        >
                                            <v-icon dark>
                                                mdi-plus
                                            </v-icon>
                                        </v-btn>
                                    </v-list-item-action>
                                </v-list-item>
                            </v-card>
                        </v-col>
                    </v-row>
                </template>

                <!-- <template v-slot:footer>
                    <v-toolbar
                        class="mt-2"
                        dark
                        flat
                    >
                        <span
                            v-show="currentCategoryRecords > 0"
                        >
                            <v-spacer></v-spacer>
                            Loaded {{ currentCategoryRecordsLoaded }} of {{ currentCategoryRecords }} total items
                        </span>
                    </v-toolbar>
                </template> -->
            </v-data-iterator>
        </v-col>
        
        <v-snackbar
            v-model="copiedToClipboardSnackbar"
            color="success"
        >
            Copied to clipboard.
            <template v-slot:action="{ attrs }">
                <v-btn
                    text
                    v-bind="attrs"
                    @click="copiedToClipboardSnackbar = false"
                >
                    Close
                </v-btn>
            </template>
        </v-snackbar>

        <v-dialog
            v-model="showItemPage"
            :fullscreen="$store.state.settings.useFullscreenDialogs"
            persistent
        >
            <transition name="fade" mode="out-in">
                <ItemPage 
                    :item="itemData"
                ></ItemPage>
            </transition>
        </v-dialog>

    </v-container>
</template>

<script>
import { EventBus } from '../plugins/event-bus.js';
import ItemPage from './Dialogs/ItemPage'
var vue_this;

export default {
    name: 'Bazaar',
    components: {
        ItemPage
    },
    props: [],
    data: () => ({
        pouchDB: null,
        bazaarData: null,
        bazaarIteratorItemsPerPage: 4,
        // Data to display in the iterator
        currentCategory: 'Select a category',
        currentCategoryRecords: 0,
        currentCategoryRecordsLoaded: 0,
        bazaarIteratorData: [],
        bazaarIteratorPage: 1,
        copiedToClipboardSnackbar: false,
        // Data to display in the ItemPage
        itemData: {},
        showItemPage: false
    }),
    created: function () {
        vue_this = this;

        // Import exported GZ file
        this.bazaarData = this.getSync(this.$store.state.settings.bazaar.repo + "/inventoryDB.gz?" + Date.now());
        // Clean out DB cache and then recreate it.
        // eslint-disable-next-line
        var destroyDB = new PouchDB('inventory');
        destroyDB.destroy().then(function () {
            console.info('Destroyed DB successfully.');
            vue_this.makeDB(vue_this.bazaarData);
            // Call to create top level categories based on DB data
            vue_this.getTopCategories();
        }).catch(function (err) {
            console.info('Failed to destroy DB: ', err);
        });
    },
    computed: {
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
        watchSelectedCategory: {
            get () {
                return this.$store.state.bazaar.selectedCategory;
            }
        }
    },
    watch: {
        watchSelectedCategory: {
            handler: function (newVal) {
                this.selectCategory(newVal);
            }
        }
    },
    methods: {
        clearDB: function () {
            
        },
        // Unzip import and load into pouchDB
        makeDB: function (data) {
            // eslint-disable-next-line
            data = pako.ungzip(data, {
                to: 'string'
            });
            console.info('this.bazaarData', JSON.parse(data));

            // eslint-disable-next-line
            this.pouchDB = new PouchDB(this.$store.state.settings.bazaar.dbName);
            
            this.pouchDB.allDocs({include_docs: true},function(err, docs) {
                if (err) {
                    console.info("Pre-read fail: " + err);
                } else {
                    console.info("Pre-read success: ", docs);
                }
            });
            
            this.pouchDB.bulkDocs(
                JSON.parse(data), {
                    new_edits: false
                }
            )
            .then(function(res) { 
                console.log('bulkDocs-success', res); 
                console.log('Finished Bazaar DB Import.');
            
                // Write DB Info to console
                vue_this.pouchDB.info().then(function (info) {
                    console.info('dbName', info.db_name, 'records', info.doc_count);
                });
            })
            .catch(function(res) { console.log('bulkDocs-failure', res); });
        },
        // GET synchronously
        getSync: function ($URL) {
            var Httpreq = new XMLHttpRequest(); // a new request
            Httpreq.open("GET", $URL, false);
            Httpreq.send(null);
            return Httpreq.responseText;
        },
        // DB Function to pull top level categories
        getTopCategories: function () {
            this.pouchDB.search({
                query: 'Bazaar',
                fields: ['parent'],
                include_docs: true
            }).then(function (res) {
                vue_this.bazaarStore.categories = [];
                res.rows.forEach (function (category) {
                    console.info('Adding top category', category);
                    vue_this.bazaarStore.categories.push(
                        {
                            'title': category.doc.name,
                        }
                    );
                });
            });
        },
        selectCategory: function (category) {
            vue_this.pouchDB.search({
                query: category,
                fields: ['type'],
                include_docs: true
            }).then(async function (data) {
                console.info('Setting category to', category, data);
                vue_this.bazaarIteratorData = [];
                vue_this.currentCategory = category;
                vue_this.currentCategoryRecords = data.total_rows;
                vue_this.currentCategoryRecordsLoaded = 0;

                for (var i = 0; i < data.total_rows; i++) {
                    console.info('Found item', data.rows[i]);
                    vue_this.pushItemToBazaar(data.rows[i].doc.path, data.rows[i].doc);
                    // var itemPath = data.rows[i].doc.path;
                    // var resourcePath = itemPath + '/resource.json';
                    // var retrievedData = vue_this.getSync(resourcePath);
                    // var parsedData;
                    
                    // window.$.ajax({
                    //     type: 'GET',
                    //     url: resourcePath,
                    //     contentType: 'application/json',
                    //     itemRootPath: data.rows[i].doc.path,
                    //     itemName: data.rows[i].doc.name,
                    //     categoryAtStart: vue_this.currentCategory 
                    // })
                    //     .done(function (result) {
                    //         // If the category is still the same when our retrieval function returns home.
                    //         if (vue_this.currentCategory === this.categoryAtStart) {
                    //             vue_this.currentCategoryRecordsLoaded++;
                    //             vue_this.pushItemToBazaar(this.itemRootPath, result);
                    //         }
                    //     })
                    //     .fail(function () {
                    //         console.info('Failed to retrieve resource.json for', this.itemName);
                    //     })
                }
            });
        },
        pushItemToBazaar: function (itemRootPath, itemData) {
            var iconToPush;
            if (itemData.icon) {
                iconToPush = this.combinePaths(itemRootPath, itemData.icon);
            } else {
                iconToPush = '';
            }

            this.bazaarIteratorData.push({
                name: itemData.name,
                description: itemData.description,
                main: this.combinePaths(itemRootPath, itemData.main),
                icon: iconToPush,
                author: itemData.author,
                image: itemData.image,
                license: itemData.license,
                parent: itemData.parent,
                path: itemData.path,
                type: itemData.type,
                version: itemData.version,
                currentTab: 0
            });
        },
        addItemToInventory: function (item) {
            this.sendEvent('add-item-from-bazaar', item);
        },
        combinePaths: function (path1, path2) {
            if (path1.endsWith('/') && !path2.startsWith('/')) {
                return path1 + path2;
            }

            if (path1.endsWith('/') && path2.startsWith('/')) {
                var trimPath2 = path2.slice(0, -1);
                return path1 + trimPath2;
            }
            
            if (!path1.endsWith('/') && !path2.startsWith('/')) {
                return path1 + '/' + path2;
            }
        },
        sendEvent: function (command, data) {
            EventBus.$emit(command, data);
        },
        copyToClipboard: function (textToCopy) {
            navigator.clipboard.writeText(textToCopy);
            this.copiedToClipboardSnackbar = true;
        }
    }
};
</script>