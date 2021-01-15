<!--
//  ShareDialog.vue
//
//  Created by Kalila L. on Jan 15 2021.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
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

            <v-select
                v-model="shareDialogStore.data.recipient"
                :items="$store.state.nearbyUsers"
                item-value="uuid"
                :rules="[v => !!v || 'A recipient is required']"
                label="Nearby Users"
                required
            >
                <template v-slot:item="data">
                    <i style="color: grey; margin-right: 5px;">{{data.item.distance.toFixed(1)}}m</i> {{data.item.name}}
                </template>
                <template v-slot:selection="data">
                    <i style="color: grey; margin-right: 5px;">{{data.item.distance.toFixed(1)}}m</i> {{data.item.name}}
                </template>
            </v-select>

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
                    :disabled="!shareDialogStore.valid"
                    @click="shareDialogStore.show = false; $emit('share-item', shareDialogStore.data.uuid);"
                >
                    Send
                </v-btn>

            </v-card-actions>
        </v-form>
    </v-card>
</template>

<script>
export default {
    name: 'ShareDialog',
    components: {
    },
    data: () => ({
    }),
    created: function () {
    },
    computed: {
        shareDialogStore: {
            get () {
                return this.$store.state.shareDialog;
            },
            set (value) {
                this.$store.commit('mutate', {
                    property: 'shareDialog', 
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