/*
    store.js

    Created by Kalila L. on 16 Apr 2020.
    Copyright 2020 Vircadia and contributors.
    
    Distributed under the Apache License, Version 2.0.
    See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
*/

import Vue from 'vue';
import Vuex from 'vuex';

Vue.use(Vuex);

export const store = new Vuex.Store({
    devtools: true,
    state: {
        items: [
            // {
            //     "hasChildren": false,
            //     "type": "script",
            //     "name": "VRGrabScale",
            //     "url": "https://gooawefaweawfgle.com/vr.js",
            //     "folder": "No Folder",
            //     "uuid": "54254354353",
            // },
            {
                "name": "Test Folder",
                "folder": "No Folder",
                "items": [
                    {
                        "name": "inception1",
                        "folder": "Test Folder",
                        "items": [
                            {
                                "name": "inception2",
                                "folder": "Test Folder",
                                "items": [
                                    {
                                        "type": "script",
                                        "name": "itemincepted",
                                        "url": "https://googfdafsgaergale.com/vr.js",
                                        "folder": "FolderWithinAFolder",
                                        "uuid": "hkjkjhkjk",
                                    },
                                ],
                                "uuid": "adsfa32"
                            },
                        ],
                        "uuid": "s4g4sg"
                    },
                ],
                "uuid": "sdfsdf",
            },
            // {
            //     "hasChildren": true,
            //     "name": "Test Folder",
            //     "folder": "No Folder",
            //     "items": [
            //         {
            //             "hasChildren": false,
            //             "type": "script",
            //             "name": "TESTFOLDERSCRIPT",
            //             "url": "https://googfdafsgaergale.com/vr.js",
            //             "folder": "Test Folder",
            //             "uuid": "54hgfhgf25fdfadf4354353",
            //         },
            //         {
            //             "hasChildren": false,
            //             "type": "script",
            //             "name": "FOLDERSCRIPT2",
            //             "url": "https://googfdafsgaergale.com/vr.js",
            //             "folder": "Test Folder",
            //             "uuid": "54hgfhgf25ffdafddfadf4354353",
            //         },
            //         {
            //             "hasChildren": true,
            //             "name": "FolderWithinAFolder",
            //             "folder": "Test Folder",
            //             "items": [
            //                 {
            //                     "hasChildren": false,
            //                     "type": "script",
            //                     "name": "inception1",
            //                     "url": "https://googfdafsgaergale.com/vr.js",
            //                     "folder": "FolderWithinAFolder",
            //                     "uuid": "54hgfhgf25fdfadeqwqeqf4354353",
            //                 },
            //                 {
            //                     "hasChildren": false,
            //                     "type": "script",
            //                     "name": "123what",
            //                     "url": "https://googfdafsgaergale.com/vr.js",
            //                     "folder": "FolderWithinAFolder",
            //                     "uuid": "54hgfhgf25ffdafdWDQDdsadasQWWQdfadf4354353",
            //                 },
            //                 {
            //                     "hasChildren": false,
            //                     "type": "script",
            //                     "name": "inception432",
            //                     "url": "https://googfdafsgaergale.com/vr.js",
            //                     "folder": "FolderWithinAFolder",
            //                     "uuid": "54hgfhgf25ffdafdWDQDQWWQdfadf4354353",
            //                 },
            //             ],
            //             "uuid": "54354363wgtrhtrhegs45ujs"
            //         },
            //     ],
            //     "uuid": "54354363wgsegs45ujs",
            // },
            // {
            //     "hasChildren": false,
            //     "type": "script",
            //     "name": "VRGrabScale",
            //     "url": "https://googfdafsgaergale.com/vr.js",
            //     "folder": "No Folder",
            //     "uuid": "54hgfhgf254354353",
            // },
            // {
            //     "hasChildren": false,
            //     "type": "script",
            //     "name": "TEST",
            //     "url": "https://gooadfdagle.com/vr.js",
            //     "folder": "No Folder",
            //     "uuid": "542rfwat4t5fsddf4354353",
            // },
            // {
            //     "hasChildren": false,
            //     "type": "json",
            //     "name": "TESTJSON",
            //     "url": "https://gooadfdagle.com/vr.json",
            //     "folder": "No Folder",
            //     "uuid": "542rfwat4t54354353",
            // },
            // {
            //     "hasChildren": false,
            //     "type": "script",
            //     "name": "TESTLONGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG",
            //     "url": "https://googfdaffle.com/vrLONGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG.js",
            //     "folder": "No Folder",
            //     "uuid": "5425ggsrg45354353",
            // },
            // {
            //     "hasChildren": false,
            //     "type": "whatttype",
            //     "name": "BrokenIcon",
            //     "url": "https://googfdaffle.com/vrLONGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG.js",
            //     "folder": "No Folder",
            //     "uuid": "5425ggsrg4fdaffdff535asdasd4353",
            // },
            // {
            //     "hasChildren": false,
            //     "type": "avatar",
            //     "name": "AVI",
            //     "url": "https://googlfadfe.com/vr.fst",
            //     "folder": "No Folder",
            //     "uuid": "542gregg45s3g4354353",
            // },
            // {
            //     "hasChildren": false,
            //     "type": "avatar",
            //     "name": "AVI",
            //     "url": "https://googlefdaf.com/vr.fst",
            //     "folder": "No Folder",
            //     "uuid": "5420798-087-54354353",
            // },
            // {
            //     "hasChildren": false,
            //     "type": "model",
            //     "name": "3D MODEL",
            //     "url": "https://googlee.com/vr.fbx",
            //     "folder": "No Folder",
            //     "uuid": "54254354980-7667jt353",
            // },
            // {
            //     "hasChildren": false,
            //     "type": "place",
            //     "name": "PLACE DOMAIN",
            //     "url": "https://googleee.com/vr.fbx",
            //     "folder": "No Folder",
            //     "uuid": "542543sg45s4gg54353",
            // },
        ],
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
                "folder": null,
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
    },
    mutations: {
        mutate (state, payload) {
            state[payload.property] = payload.with;
            console.info("Payload:", payload.property, "with:", payload.with, "state is now:", this.state);
        },
        sortTopInventory (state) {
            state.items.sort(function(a, b) {
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
        },
        moveFolder (state, payload) {
            
            let { items } = state;
            
            if (payload.parentFolderUUID === "top") {
                payload.findFolder.returnedItem.folder = "No Folder";
                console.info("Going to push...", payload.findFolder.returnedItem);
                console.info("Containing these items...", payload.findFolder.returnedItem.items);

                items.push(payload.findFolder.returnedItem);
                Vue.set(state,'items', items)
                                
            } else if (payload.findParentFolder) {                
                // console.info("Going to push...", payload.findFolder.returnedItem);
                // console.info("Containing these items...", payload.findFolder.returnedItem.items);
                // console.info("Into...", payload.findParentFolder.returnedItem);

                payload.findFolder.returnedItem.folder = payload.findParentFolder.name;
                payload.findParentFolder.returnedItem.items.push(payload.findFolder.returnedItem);
                Vue.set(state,'items', items)
            }
        },
    }
})