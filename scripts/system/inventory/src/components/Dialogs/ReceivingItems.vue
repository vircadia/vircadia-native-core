<!--
//  ReceivingItems.vue
//
//  Created by Kalila L. on Jan 15 2021.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <v-card>
        <v-card-title class="headline">
            Item Inbox
        </v-card-title>

        <v-card-text v-show="receivingItemsDialogStore.data.receivingItemQueue.length > 0">
            A list of all items being received currently.
        </v-card-text>
        
        <v-card-text v-show="receivingItemsDialogStore.data.receivingItemQueue.length === 0">
            There are currently no items in your inbox.
        </v-card-text>

        <v-card-actions>
            <v-list
                nav
                class="pt-5"
                max-width="370"
                v-show="receivingItemsDialogStore.data.receivingItemQueue.length > 0"
            >

                <v-list-item
                    two-line
                    v-for="item in receivingItemsDialogStore.data.receivingItemQueue" 
                    v-bind:key="item.data.uuid"
                >
                    <v-list-item-content>
                        <v-list-item-title>{{item.data.name}}</v-list-item-title>
                        <v-list-item-subtitle>Sent by {{item.senderName}}</v-list-item-subtitle>
                        <v-list-item-subtitle>Distance: {{item.senderDistance.toFixed(1)}}m</v-list-item-subtitle>
                    </v-list-item-content>
                        <v-btn color="success" @click="$emit('accept-receiving-item', item)">
                            <v-icon>mdi-plus</v-icon>
                        </v-btn>
                        <v-btn text color="red" @click="$emit('remove-receiving-item', item.data.uuid)">
                            <v-icon>mdi-minus</v-icon>
                        </v-btn>
                </v-list-item>

            </v-list>
        </v-card-actions>
    </v-card>
</template>

<script>
export default {
    name: 'ReceivingItems',
    components: {
    },
    data: () => ({
    }),
    created: function () {
    },
    computed: {
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
        }
    },
    watch: {
    },
    methods: {
    }
};
</script>