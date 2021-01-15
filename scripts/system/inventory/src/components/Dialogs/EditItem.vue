<!--
//  EditItem.vue
//
//  Created by Kalila L. on Jan 15 2021.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <v-card>
        <v-card-title class="headline">Edit Item</v-card-title>

        <v-form
            ref="editForm"
            v-model="editDialogStore.valid"
            :lazy-validation="false"
        >

            <v-select
                :items="$store.state.supportedItemTypes"
                class="my-2"
                v-model="editDialogStore.data.type"
                :rules="[v => !!v || 'Type is required.']"
                label="Item Type"
                outlined
            ></v-select>
            
            <v-select
                :items="$store.state.folderList"
                item-text="name"
                item-value="uuid"
                class="my-2"
                v-model="editDialogStore.data.folder"
                label="Folder"
                outlined
            ></v-select>

            <v-text-field
                class="px-2"
                label="Name"
                v-model="editDialogStore.data.name"
                :rules="[v => !!v || 'Name is required.']"
                required
            ></v-text-field>

            <v-text-field
                class="px-2"
                label="URL"
                v-model="editDialogStore.data.url"
                :rules="[v => !!v || 'URL is required.']"
                required
            ></v-text-field>
            
            <v-text-field
                class="px-2"
                label="Version"
                v-model="editDialogStore.data.version"
                :rules="[v => !!v || 'Version is required.']"
                required
            ></v-text-field>
            
            <v-combobox
                class="px-2"
                label="Tags"
                v-model="editDialogStore.data.tags"
                multiple
                :chips="true"
                :deletable-chips="true"
                :disable-lookup="true"
            ></v-combobox>
            
            <v-textarea
                class="px-2"
                label="Metadata"
                v-model="editDialogStore.data.metadata"
            ></v-textarea>

            <v-card-actions>

                <v-btn
                    color="red"
                    class="px-3"
                    @click="editDialogStore.show = false"
                >
                    Cancel
                </v-btn>

                <v-spacer></v-spacer>

                <v-btn
                    color="blue"
                    class="px-3"       
                    :disabled="!editDialogStore.valid"             
                    @click="editDialogStore.show = false; $emit('edit-item', editDialogStore.uuid);"
                >
                    Done
                </v-btn>

            </v-card-actions>
        </v-form>
    </v-card>
</template>

<script>
export default {
    name: 'EditItem',
    components: {
    },
    data: () => ({
    }),
    created: function () {
    },
    computed: {
        editDialogStore: {
            get () {
                return this.$store.state.editDialog;
            },
            set (value) {
                this.$store.commit('mutate', {
                    property: 'editDialog', 
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