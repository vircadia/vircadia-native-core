<!--
//  Index.vue
//
//  Created by Kalila L. on Sep. 4th, 2021.
//  Copyright 2021 Vircadia contributors.
//  Copyright 2021 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <div>
        <NodesList></NodesList>

        <AssignmentsList></AssignmentsList>
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import NodesList from "@Components/dashboard/Nodes";
import AssignmentsList from "@Components/dashboard/Assignments";

export default defineComponent({
    name: "Index",

    components: {
        NodesList,
        AssignmentsList
    },

    methods: {
    }
});
</script>
