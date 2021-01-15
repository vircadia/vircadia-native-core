<!--
//  EditFolder.vue
//
//  Created by Kalila L. on Jan 15 2021.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <v-card>
        <v-card-title class="headline">Edit Folder</v-card-title>
        
        <v-form
            ref="editFolderForm"
            v-model="editFolderDialogStore.valid"
            :lazy-validation="false"
        >
    
            <v-text-field
                class="px-2"
                label="Name"
                v-model="editFolderDialogStore.data.name"
                :rules="[v => !!v || 'Name is required.']"
                required
            ></v-text-field>
            
            <v-select
                :items="$store.state.folderList"
                item-text="name"
                item-value="uuid"
                class="my-2"
                v-model="editFolderDialogStore.data.folder"
                label="Parent Folder"
                outlined
            ></v-select>
    
            <v-card-actions>
    
                <v-btn
                    color="red"
                    class="px-3"
                    @click="editFolderDialogStore.show = false"
                >
                    Cancel
                </v-btn>
                
                <v-spacer></v-spacer>
                
                <v-btn
                    color="blue"
                    class="px-3"       
                    :disabled="!$store.state.editFolderDialog.valid"             
                    @click="editFolderDialogStore.show = false; $emit('edit-folder', editFolderDialogStore.uuid);"
                >
                    Done
                </v-btn>
            
            </v-card-actions>
        </v-form>
    </v-card>
</template>

<script>
export default {
    name: 'EditFolder',
    components: {
    },
    data: () => ({
    }),
    created: function () {
    },
    computed: {
        editFolderDialogStore: {
            get () {
                return this.$store.state.editFolderDialog;
            },
            set (value) {
                this.$store.commit('mutate', {
                    property: 'editFolderDialog', 
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