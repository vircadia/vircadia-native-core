<!--
//  Content.vue
//
//  Created by Eugene :) on Jan. 23th, 2023.
//  Copyright 2023 Vircadia contributors.
//  Copyright 2023 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <div class="q-pa-md">
        <InstalledContent/>
        <ContentArchives/>
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import InstalledContent from "@Components/settings/InstalledContent.vue";
import ContentArchives from "@Components/settings/ContentArchives.vue";

export default defineComponent({
    name: "Index",
    components: {
        InstalledContent,
        ContentArchives
    }
});
</script>
