<!--
//  CreateFolder.vue
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
            Create Folder
        </v-card-title>

        <v-card-text>
            Enter the name of the folder.
        </v-card-text>

        <v-form
            ref="createFolderForm"
            v-model="createFolderDialogStore.valid"
            :lazy-validation="false"
        >

            <v-text-field
                class="px-2"
                label="Name"
                v-model="createFolderDialogStore.data.name"
                :rules="[v => !!v || 'Name is required.']"
                required
            ></v-text-field>

            <v-card-actions>

                <v-btn
                    color="red"
                    class="px-3"
                    @click="createFolderDialogStore.show = false"
                >
                    Cancel
                </v-btn>

                <v-spacer></v-spacer>

                <v-btn
                    color="blue"
                    class="px-3"
                    :disabled="!createFolderDialogStore.valid"
                    @click="createFolderDialogStore.show = false; $emit('create-folder', createFolderDialogStore.data.name)"
                >
                    Create
                </v-btn>

            </v-card-actions>
        </v-form>
    </v-card>
</template>

<script>
export default {
    name: 'CreateFolder',
    components: {
    },
    data: () => ({
    }),
    created: function () {
    },
    computed: {
        createFolderDialogStore: {
            get () {
                return this.$store.state.createFolderDialog;
            },
            set (value) {
                this.$store.commit('mutate', {
                    property: 'createFolderDialog', 
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