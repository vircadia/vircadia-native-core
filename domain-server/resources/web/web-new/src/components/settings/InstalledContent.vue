<template>
        <!-- *END* Description Settings *END* -->
    <!-- Installed Content -->
    <q-card class="my-card">
        <q-card-section>
            <div class="text-h5 text-center text-weight-bold q-mb-sm">Installed Content</div>
            <q-separator />
            <q-card>
                <!-- Installed Content Table section -->
                <q-card-section>
                    <q-table dark class="bg-grey-9" :rows="rows" hide-pagination separator="vertical">
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
                                <q-td>{{ installedContentName }}</q-td>
                                <q-td>{{ installedContentFileName }}</q-td>
                                <q-td>{{ installedContentCreationTime }}</q-td>
                                <q-td>{{ installedContentInstallTime }}</q-td>
                                <q-td>{{ installedContentInstalledBy }}</q-td>
                            </q-tr>
                        </template>
                    </q-table>
                </q-card-section>
            </q-card>
        </q-card-section>
    </q-card>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import moment from "moment";
import { ContentSettings } from "@Modules/domain/contentSettings";

export default defineComponent({
    name: "InstalledContent",
    data () {
        return {
            rows: [{}],
            installedContentName: "",
            installedContentFileName: "",
            installedContentCreationTime: "",
            installedContentInstallTime: "",
            installedContentInstalledBy: ""
        };
    },
    methods: {
        async intializeInstalledContentSettings (): Promise<void> {
            const values = await ContentSettings.getValues();
            this.installedContentName = values.installed_content?.name ?? "";
            this.installedContentFileName = values.installed_content?.filename ?? "";
            this.installedContentCreationTime = values.installed_content?.creation_time ? moment(values.installed_content?.creation_time).format("lll") : "";
            this.installedContentInstallTime = values.installed_content?.install_time ? moment(values.installed_content?.install_time).format("lll") : "";
            this.installedContentInstalledBy = values.installed_content?.installed_by ?? "";
        }
    },
    beforeMount () {
        this.intializeInstalledContentSettings();
    }
});
</script>
