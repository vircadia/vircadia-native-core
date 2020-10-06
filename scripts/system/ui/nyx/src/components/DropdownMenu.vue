<!--
//  DropdownMenu.vue
//
//  Created by Kalila L. on 6 Oct 2020.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->
<template>
    <v-list dense>
        <v-list-item-group
            v-model="item"
            color="primary"
        >
            <v-list-item
                v-for="(item, i) in items"
                :key="i"
            >
                <v-list-item-content>
                    <v-list-item-title v-text="item"></v-list-item-title>
                </v-list-item-content>
            </v-list-item>
        </v-list-item-group>
    </v-list>
</template>

<script>

export default {
    name: 'DropdownMenu',
    methods: {
        
    },
    data: () => ({
        items: [
            'Test',
            'Waifu',
            'Lol'
        ]
    })
};

</script>