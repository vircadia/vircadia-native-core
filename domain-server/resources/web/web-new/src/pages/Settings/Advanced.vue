<!--
//  Advanced.vue
//
//  Created by Eugene :) on Jan. 12th, 2023.
//  Copyright 2023 Vircadia contributors.
//  Copyright 2023 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <div class="q-pa-md">
        <DescriptionSettings/>
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";

import DescriptionSettings from "@Components/settings/Description.vue";

export default defineComponent({
    name: "Index",
    components: {
        DescriptionSettings
    }
});
</script>
