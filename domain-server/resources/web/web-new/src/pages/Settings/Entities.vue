<!--
//  Entities.vue
//
//  Created by Eugene :) on Jan. 10th, 2023.
//  Copyright 2023 Vircadia contributors.
//  Copyright 2023 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <div class="q-pa-md">
        <EntitiesSettings/>
        <AssetServerSettings/>
        <EntityScriptServerSettings/>
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";

import EntitiesSettings from "@Components/settings/Entities.vue";
import AssetServerSettings from "@Components/settings/AssetServer.vue";
import EntityScriptServerSettings from "@Components/settings/EntityScriptServer.vue";

export default defineComponent({
    name: "Index",
    components: {
        EntitiesSettings,
        AssetServerSettings,
        EntityScriptServerSettings
    }
});
</script>
