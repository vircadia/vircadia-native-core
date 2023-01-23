<template>
    <div>
        <!-- Installed Content -->
        <q-card class="my-card">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold q-mb-sm">Installed Content</div>
                <q-separator />
                    <q-card>
                        <!-- Installed Content Table section -->
                        <q-card-section>
                            <q-table dark class="bg-grey-9" :rows="rows">
                                <template v-slot:header>
                                    <q-tr class="bg-primary">
                                        <q-th class="text-left">Name</q-th>
                                        <q-th class="text-left">File Name</q-th>
                                        <q-th class="text-left">Created</q-th>
                                        <q-th class="text-left">Installed</q-th>
                                        <q-th class="text-left">Installed By</q-th>
                                    </q-tr>
                                </template>
                                <template v-slot:body>
                                    <q-tr>
                                        <q-td>{{ installedContent.name }}</q-td>
                                        <q-td>{{ installedContent.filename }}</q-td>
                                        <q-td>{{ installedContent.creation_time }}</q-td>
                                        <q-td>{{ installedContent.install_time }}</q-td>
                                        <q-td>{{ installedContent.installed_by }}</q-td>
                                    </q-tr>
                                </template>
                            </q-table>
                        </q-card-section>
                    </q-card>
            </q-card-section>
        </q-card>
        <!-- *END* Description Settings *END* -->
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { InstalledContent } from "@Modules/domain/interfaces/contentSettings";
import { ContentSettings } from "@Modules/domain/contentSettings";

export default defineComponent({
    name: "InstalledContent",

    data () {
        return {
            rows: [{}],
            installedContent: {} as InstalledContent
        };
    },
    methods: {
        async refreshSettingsValues (): Promise<void> {
            const values = await ContentSettings.getValues();
            this.installedContent = values.installed_content ?? {} as InstalledContent;
        }
    },
    beforeMount () {
        this.refreshSettingsValues();
    }
});
</script>
