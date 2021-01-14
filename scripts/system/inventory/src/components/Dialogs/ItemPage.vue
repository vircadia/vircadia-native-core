<!--
//  ItemPage.vue
//
//  Created by Kalila L. on Jan 13 2021.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <v-card
        :loading="itemPageStore.loading"
        class="mx-auto my-12"
        max-width="450"
    >
        <template slot="progress">
            <v-progress-linear
            color="deep-purple"
            height="10"
            indeterminate
            ></v-progress-linear>
        </template>

        <v-card-title class="white--text mt-8">
            <v-avatar size="56">
                <img
                    alt="Icon"
                    :src="itemPageStore.data.icon"
                >
            </v-avatar>
            <p class="ml-3">
                {{ itemPageStore.data.name }}
            </p>
            <v-spacer></v-spacer>
            <v-btn
                class="mx-2"
                fab
                color="error"
                small
                @click="itemPageStore.show = false"
            >
                <v-icon dark>
                    mdi-close
                </v-icon>
            </v-btn>
        </v-card-title>
        
        <v-card-actions>
            <v-btn
                color="primary"
                block
                @click="addItemToInventory(item)"
            >
                <v-icon dark>
                    mdi-plus
                </v-icon>
                Add to Inventory
            </v-btn>
        </v-card-actions>
        <v-card-actions>
            <v-btn
                color="success"
                style="width: 49%"
            >
                <v-icon dark>
                    mdi-play
                </v-icon>
                Use
            </v-btn>
            <v-btn
                color="primary"
                style="width: 49%"
            >
                <v-icon dark>
                    mdi-share
                </v-icon>
                Share
            </v-btn>
        </v-card-actions>
        
        <v-divider class="mt-2"></v-divider>
        
        <v-sheet
            class="mx-auto"
        >
            <v-slide-group
                show-arrows
                v-show="true"
            >
                <v-slide-item
                    v-for="(item, i) in itemPageStore.data.image"
                    :key="i"
                    class="mx-0"
                >
                    <v-img
                        :src="item"
                        max-width="250"
                        max-height="150"
                        class="mx-0"
                    ></v-img>
                </v-slide-item>
            </v-slide-group>
        </v-sheet>
        
        <v-divider class="mb-2"></v-divider>

        <v-card-text
            class="mt-4"
        >
            <div>
                {{ itemPageStore.data.description }} 
            </div>

            <v-divider class="my-2"></v-divider>
            
            <div class="grey--text">
                <b>Authored by</b> {{ itemPageStore.data.author }}
            </div>
            
            <v-divider class="my-2"></v-divider>
            
            <div class="grey--text">
                <b>Version</b> {{ itemPageStore.data.version }}
            </div>
            
            <v-divider class="my-2"></v-divider>
            
            <div
                v-if="itemPageStore.data.tags.length > 0"
                class="grey--text"
            >
                <b>Tags</b> • {{ itemPageStore.data.tags.join(", ") }}
            </div>
        </v-card-text>

        <v-divider class=""></v-divider>

        <v-expansion-panels>
            <v-expansion-panel
                :disabled="itemPageStore.data.sublicense.length === 0"
            >
                <v-expansion-panel-header>
                    License • {{ itemPageStore.data.license }}
                </v-expansion-panel-header>
                <v-expansion-panel-content
                    v-if="itemPageStore.data.sublicense.length > 0"
                >
                    <v-list-item 
                        v-for="sublicense in itemPageStore.data.sublicense"
                        :key="sublicense.name"
                    >
                        <v-list-item-content>
                            <v-list-item-title>{{ sublicense.name }}</v-list-item-title>
                            <v-list-item-subtitle>
                                {{ sublicense.license }}
                            </v-list-item-subtitle>
                            <v-list-item-subtitle>
                                {{ sublicense.email }}
                            </v-list-item-subtitle>
                            <v-list-item-subtitle>
                                {{ sublicense.url }}
                            </v-list-item-subtitle>
                        </v-list-item-content>
                    </v-list-item>
                </v-expansion-panel-content>
            </v-expansion-panel>
        </v-expansion-panels>
    </v-card>
</template>

<script>

import { EventBus } from '../../plugins/event-bus.js';

export default {
    name: 'ItemPage',
    components: {
    },
    data: () => ({
        // Modules
        path: null
    }),
    created: function () {
        this.path = require('path')
    },
    computed: {
        itemPageStore: {
            get() {
                return this.$store.state.itemPage;
            },
            set (value) {
                this.$store.commit('mutate', {
                    update: true,
                    property: 'itemPage', 
                    with: value
                });
            }
        }
    },
    watch: {
    },
    methods: {
        sendEvent: function (command, data) {
            EventBus.$emit(command, data);
        },
        addItemToInventory: function (item) {
            this.sendEvent('add-item-from-bazaar', item);
        }
    }
};
</script>