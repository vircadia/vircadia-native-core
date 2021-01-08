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
                            <!-- <v-breadcrumbs :items="items"></v-breadcrumbs> -->
                        </v-toolbar-title>
                        <v-btn
                            v-for="subCategory in subCategoryStore"
                            :key="subCategory.title"
                            color="primary"
                            class="mx-1"
                        >
                            {{ subCategory.title }}
                        </v-btn>
                    </v-toolbar>
                </template>

                <template v-slot:default="props">
                    <v-row>
                        <v-col
                            v-for="item in props.items"
                            :key="item.name"
                            cols="12"
                            sm="6"
                            md="4"
                            lg="3"
                        >
                            <v-card>
                                <v-card-title class="subheading font-weight-bold">
                                    <v-avatar
                                        class="mr-2"
                                    >
                                        <img
                                            :src="item.icon"
                                            alt="Preview"
                                        >
                                    </v-avatar>
                                    {{ item.name }}

                                    <v-spacer></v-spacer>

                                    <v-btn
                                        class="mx-2"
                                        fab
                                        color="primary"
                                        small
                                        @click="console.log(item.main)"
                                    >
                                        <v-icon dark>
                                            mdi-plus
                                        </v-icon>
                                    </v-btn>
                                </v-card-title>
                                
                                <v-tabs
                                    fixed-tabs
                                    v-model="item.currentTab"
                                    background-color="deep-purple accent-4"
                                    dark
                                >
                                    <v-tabs-slider></v-tabs-slider>

                                    <v-tab @click="item.currentTab = 0">
                                        <v-icon>mdi-information-variant</v-icon>
                                    </v-tab>

                                    <v-tab @click="item.currentTab = 1">
                                        <v-icon>mdi-link</v-icon>
                                    </v-tab>
                                </v-tabs>

                                <v-tabs-items v-model="item.currentTab">
                                    <v-tab-item>
                                        <v-card-text>
                                            {{ item.description }}
                                        </v-card-text>
                                    </v-tab-item>
                                    <v-tab-item>
                                        <v-card-text>
                                            <!-- <v-subheader></v-subheader> -->
                                            <v-list-item class="mb-4">
                                                <kbd
                                                    class="text-center mt-4" 
                                                    style="font-size: 1.0rem !important" 
                                                    v-text="item.main"
                                                >
                                                </kbd>
                                                <v-tooltip left>
                                                    <template v-slot:activator="{ on, attrs }">
                                                        <v-btn
                                                            v-bind="attrs"
                                                            v-on="on"
                                                            @click="copyToClipboard(item.main)" 
                                                            color="input"
                                                            class="ml-3 mt-3"
                                                            small
                                                            fab
                                                        >
                                                            <v-icon>mdi-content-copy</v-icon>
                                                        </v-btn>
                                                    </template>
                                                    <span>Copy</span>
                                                </v-tooltip>
                                            </v-list-item>
                                        </v-card-text>
                                    </v-tab-item>
                                </v-tabs-items>
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
        
        <v-bottom-navigation
            app
        >
            <span>
                <v-select
                    :items="categoryStore"
                    item-text="title"
                    item-value="title"
                    label="Category"
                    class=""
                    @change="selectCategory"
                ></v-select>
            </span>
            <div
                v-show="currentCategoryRecords > 0"
                class="ml-6 mt-4"
            >
                Loaded {{ currentCategoryRecordsLoaded }} of {{ currentCategoryRecords }} total items
            </div>
        </v-bottom-navigation>
        
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
    </v-container>
</template>

<script>
import { EventBus } from '../plugins/event-bus.js';
var vue_this;

export default {
    name: 'Bazaar',
    components: {
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
        copiedToClipboardSnackbar: false
    }),
    created: function () {
        vue_this = this;

        // Import exported GZ file
        this.bazaarData = this.getSync(this.$store.state.settings.bazaar.repo + "/inventoryDB.gz?" + Date.now());
        // Clear the DB out then reload it...
        this.clearDB.then(function (result) {
            if (result === true) {
                console.info('destruct successful.');
                vue_this.makeDB(this.bazaarData);
                // Call to create top level categories based on DB data
                this.getTopCategories();
            } else {
                console.info('destruct failed:', result);
            }
        });
    },
    computed: {
        categoryStore: {
            get() {
                return this.$store.state.categoryStore;
            },
            set (value) {
                this.$store.commit('mutate', {
                    property: 'categoryStore', 
                    with: value
                });
            }
        },
        subCategoryStore: {
            get() {
                return this.$store.state.subCategoryStore;
            },
            set (value) {
                this.$store.commit('mutate', {
                    property: 'subCategoryStore', 
                    with: value
                });
            }
        }
    },
    watch: {
    },
    methods: {
        clearDB: function () {
            // eslint-disable-next-line
            return new PouchDB(this.$store.state.settings.bazaar.dbName).destroy();
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
        getTopCategories: function() {
            this.pouchDB.search({
                query: 'Bazaar',
                fields: ['parent'],
                include_docs: true
            }).then(function (res) {
                vue_this.categoryStore = [];
                res.rows.forEach (function (category) {
                    console.info('Setting top category', category);
                    vue_this.categoryStore.push(
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
                fields: ['parent'],
                include_docs: true
            }).then(async function (data) {
                console.info('Setting category to', category, data);
                vue_this.bazaarIteratorData = [];
                vue_this.currentCategory = category;
                vue_this.currentCategoryRecords = data.total_rows;
                vue_this.currentCategoryRecordsLoaded = 0;

                for (var i = 0; i < data.total_rows; i++) {
                    console.info('Found item', data.rows[i]);
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
        pushItemToBazaar: function(itemRootPath, itemData) {
            this.bazaarIteratorData.push({
                name: itemData.name,
                description: itemData.description,
                main: this.combinePaths(itemRootPath, itemData.main),
                icon: this.combinePaths(itemRootPath, itemData.icon),
                currentTab: 0
            });
        },
        combinePaths: function(path1, path2) {
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