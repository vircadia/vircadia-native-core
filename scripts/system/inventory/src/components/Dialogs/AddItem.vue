<!--
//  AddItem.vue
//
//  Created by Kalila L. on Jan 15 2021.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <v-card>
        <v-card-title class="headline">Add Item</v-card-title>

        <v-form
            ref="addForm"
            v-model="addDialogStore.valid"
            :lazy-validation="false"
        >

            <v-card-text>
                Enter the name of the item.
            </v-card-text>

            <v-text-field
                class="px-2"
                label="Name"
                v-model="addDialogStore.data.name"
                :rules="[v => !!v || 'Name is required.']"
                required
            ></v-text-field>

            <v-card-text>
                Select a folder (optional).
            </v-card-text>

            <v-select
                class="my-2"
                :items="$store.state.folderList"
                v-model="addDialogStore.data.folder"
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
                v-model="addDialogStore.data.url"
                :rules="[v => !!v || 'URL is required.']"
                required
            ></v-text-field>
            
            <v-text-field
                class="px-2"
                label="Version"
                v-model="addDialogStore.data.version"
                :rules="[v => !!v || 'Version is required.']"
                required
            ></v-text-field>

            <v-combobox
                class="px-2"
                label="Tags"
                v-model="addDialogStore.data.tags"
                multiple
                :chips="true"
                :deletable-chips="true"
                :disable-lookup="true"
            ></v-combobox>

            <v-textarea
                class="px-2"
                label="Metadata"
                v-model="addDialogStore.data.metadata"
            ></v-textarea>

            <v-card-actions>

                <v-btn
                    color="red"
                    class="px-3"
                    @click="addDialogStore.show = false"
                >
                    Cancel
                </v-btn>

                <v-spacer></v-spacer>

                <v-btn
                    color="blue"
                    class="px-3"
                    :disabled="!$store.state.addDialog.valid"
                    @click="addDialogStore.show = false; $emit('add-item')"
                >
                    Add
                </v-btn>

            </v-card-actions>
        </v-form>
    </v-card>
</template>

<script>
export default {
    name: 'AddItem',
    components: {
    },
    data: () => ({
    }),
    created: function () {
    },
    computed: {
        addDialogStore: {
            get () {
                return this.$store.state.addDialog;
            },
            set (value) {
                this.$store.commit('mutate', {
                    property: 'addDialog', 
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