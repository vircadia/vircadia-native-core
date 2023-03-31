<!--
//  Advanced.vue
//
//  Created by Eugene :) on Jan. 17th, 2023.
//  Copyright 2023 Vircadia contributors.
//  Copyright 2023 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <div class="q-pa-sm row">
        <BackupRestoreSettings class="col-12 col-md"/>
        <AutomaticContentArchivesSettings class="col-12 col-md"/>
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import AutomaticContentArchivesSettings from "@Components/settings/AutomaticContentArchives.vue";
import BackupRestoreSettings from "@Components/settings/SettingsBackupRestore.vue";

export default defineComponent({
    name: "Index",
    components: {
        AutomaticContentArchivesSettings,
        BackupRestoreSettings
    }
});
</script>
