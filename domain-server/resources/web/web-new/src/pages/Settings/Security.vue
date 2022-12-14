<!--
//  Security.vue
//
//  Created by Eugene :) on Dec. 13th, 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <div class="q-pa-md">
        <SSLClientSettings/>
        <MonitoringSettings/>
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import SSLClientSettings from "@Components/settings/SSLClient.vue";
import MonitoringSettings from "@Components/settings/Monitoring.vue";

export default defineComponent({
    name: "Networking",
    components: {
        SSLClientSettings,
        MonitoringSettings
    }
});
</script>
