<!--
//  ReceiveItem.vue
//
//  Created by Kalila L. on Jan 15 2021.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <v-card>
        <v-card-title class="headline">Receiving Item</v-card-title>

        <v-card-text>
            <b>{{ receiveDialogStore.userDisplayName }} sent you an item.</b> <br />
            <i class="caption">User UUID: {{ receiveDialogStore.userUUID }}</i>
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
                :items="$store.state.folderList"
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
            
            <v-text-field
                class="px-2"
                label="Version"
                v-model="receiveDialogStore.data.version"
                :rules="[v => !!v || 'Version is required.']"
                required
            ></v-text-field>

            <v-combobox
                class="px-2"
                label="Tags"
                v-model="receiveDialogStore.data.tags"
                multiple
                :chips="true"
                :deletable-chips="true"
                :disable-lookup="true"
            ></v-combobox>

            <v-textarea
                class="px-2"
                label="Metadata"
                v-model="receiveDialogStore.data.metadata"
            ></v-textarea>

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
                    :disabled="!receiveDialogStore.valid"
                    @click="receiveDialogStore.show = false; $emit('confirm-item-receipt')"
                >
                    Accept
                </v-btn>

            </v-card-actions>
        </v-form>
    </v-card>
</template>

<script>
export default {
    name: 'ReceiveItem',
    components: {
    },
    data: () => ({
    }),
    created: function () {
    },
    computed: {
        receiveDialogStore: {
            get() {
                return this.$store.state.receiveDialog;
            },
            set (value) {
                this.$store.commit('mutate', {
                    property: 'receiveDialog', 
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