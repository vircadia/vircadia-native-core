<!--
//  Index.vue
//
//  Created by Kalila L. on Sep. 4th, 2021.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <div>
        <h1>Test2</h1>

        <div v-for="node in nodesList">
            {{node.uuid}}
        </div>
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { Nodes, Node } from "@Modules/domain/nodes";

export default defineComponent({
    name: "Index",

    data () {
        return {
            nodesList: [] as Node[]
        };
    },

    methods: {
        async loadNodesList (): Promise<void> {
            const nodesResult = await Nodes.getNodes();
            this.nodesList = nodesResult;
        }
    },

    mounted: async function (): Promise<void> {
        await this.loadNodesList();
    }
});
</script>
