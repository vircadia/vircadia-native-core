<!--
//  Audio.vue
//
//  Created by Eugene :) on Dec. 21th, 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <div class="q-pa-sm row">
        <AudioThreadingSettings class="col-12 col-md"/>
        <AudioEnvironmentSettings class="col-12 col-md"/>
        <AudioBuffersSettings class="col-12 col-md"/>
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";

import AudioThreadingSettings from "@Components/settings/AudioThreading.vue";
import AudioEnvironmentSettings from "@Components/settings/AudioEnvironment.vue";
import AudioBuffersSettings from "@Components/settings/AudioBuffers.vue";

export default defineComponent({
    name: "Index",
    components: {
        AudioThreadingSettings,
        AudioEnvironmentSettings,
        AudioBuffersSettings
    }
});
</script>
