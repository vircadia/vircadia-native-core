<!--
//  Networking.vue
//
//  Created by Eugene :) on Nov. 24th, 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <div class="q-pa-md">
        <MetaverseSettings/>
        <WebRTCSettings/>
        <WordpressSettings/>
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";

import MetaverseSettings from "@Components/settings/NetworkingMetaverse.vue";
import WebRTCSettings from "@Components/settings/NetworkingWebRTC.vue";
import WordpressSettings from "@Components/settings/NetworkingWordpress.vue";

export default defineComponent({
    name: "Networking",

    components: {
        MetaverseSettings,
        WebRTCSettings,
        WordpressSettings
    },

    methods: {
    }
});
</script>
