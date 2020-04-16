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
        mutate(state, payload) {
            state[payload.property] = payload.with;
            console.info("Payload:", payload.property, "with:", payload.with, "state is now:", this.state);
        }
    }
})