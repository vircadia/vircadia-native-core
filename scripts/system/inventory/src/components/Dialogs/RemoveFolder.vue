<!--
//  RemoveFolder.vue
//
//  Created by Kalila L. on Jan 15 2021.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <v-card>
        <v-card-title class="headline">Remove Folder</v-card-title>

        <v-card-text>
            Are you sure you want to delete this folder <b>and</b> all items within it from your inventory?
        </v-card-text>

        <v-card-actions>

            <v-btn
                color="blue"
                class="px-3"
                @click="removeFolderDialogStore.show = false"
            >
                No
            </v-btn>

            <v-spacer></v-spacer>

            <v-btn
                color="red"
                class="px-3"                    
                @click="removeFolderDialogStore.show = false; $emit('remove-folder', removeFolderDialogStore.uuid);"
            >
                Yes
            </v-btn>
        </v-card-actions>
    </v-card>
</template>

<script>
export default {
    name: 'RemoveFolder',
    components: {
    },
    data: () => ({
    }),
    created: function () {
    },
    computed: {
        removeFolderDialogStore: {
            get () {
                return this.$store.state.removeFolderDialog;
            },
            set (value) {
                this.$store.commit('mutate', {
                    property: 'removeFolderDialog', 
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