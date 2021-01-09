<!--
//  CategoryDrawer.vue
//
//  Created by Kalila L. on January 9 2021.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <v-container fluid>
        <v-navigation-drawer
            v-model="bazaarStore.categoryDrawer"
            fixed
            temporary
        >
            <v-list
                nav
                class="pt-5"
            >
                <v-list-item 
                    @click="bazaarStore.selectedCategory = subCategory.title; bazaarStore.categoryDrawer = false;"
                    v-for="subCategory in bazaarStore.subCategories"
                    :key="subCategory.title"
                    color="primary"
                    class="mx-1"
                >
                    <v-list-item-icon>
                        <v-icon>mdi-briefcase-outline</v-icon>
                    </v-list-item-icon>
                    <v-list-item-title>{{ subCategory.title }}</v-list-item-title>
                </v-list-item>
            </v-list>
        </v-navigation-drawer>
    </v-container>
</template>

<script>
import { EventBus } from '../plugins/event-bus.js';

export default {
    name: 'CategoryDrawer',
    components: {
    },
    props: [],
    data: () => ({
        drawer: false
    }),
    created: function () {
    },
    computed: {
        bazaarStore: {
            get() {
                return this.$store.state.bazaar;
            },
            set (value) {
                this.$store.commit('mutate', {
                    property: 'bazaar', 
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
        }
    }
};
</script>