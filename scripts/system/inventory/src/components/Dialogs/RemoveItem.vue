<!--
//  RemoveItem.vue
//
//  Created by Kalila L. on Jan 14 2021.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <v-card>
        <v-card-title class="headline">Remove Item</v-card-title>

        <v-card-text>
            Are you sure you want to delete this item from your inventory?
        </v-card-text>

        <v-card-actions>

            <v-btn
                color="blue"
                class="px-3"
                @click="removeDialogStore.show = false"
            >
                No
            </v-btn>

            <v-spacer></v-spacer>

            <v-btn
                color="red"
                class="px-3"                    
                @click="removeDialogStore.show = false; $emit('remove-item', removeDialogStore.uuid);"
            >
                Yes
            </v-btn>
        </v-card-actions>
    </v-card>
</template>

<script>
export default {
    name: 'RemoveItem',
    components: {
    },
    data: () => ({
    }),
    created: function () {
    },
    computed: {
        removeDialogStore: {
            get () {
                return this.$store.state.removeDialog;
            },
            set (value) {
                this.$store.commit('mutate', {
                    property: 'removeDialog', 
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