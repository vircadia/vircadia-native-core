<!--
//  Avatars.vue
//
//  Created by Eugene :) on Dec. 23th, 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <div class="q-pa-md">
        <AvatarsSettings/>
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";

import AvatarsSettings from "@Components/settings/Avatars.vue";

export default defineComponent({
    name: "Index",
    components: {
        AvatarsSettings
    }
});
</script>
