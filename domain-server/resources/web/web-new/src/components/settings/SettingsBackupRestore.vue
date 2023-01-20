<template>
    <q-card class="my-card q-pa-md">
        <p class="q-mb-xs text-body1 text-weight-bold">Upload a settings configuration to quickly configure this domain</p>
        <div class="text-caption text-warning">Note: Your domain settings will be replaced by the settings you upload.</div>
        <div style="display: flex; justify-content: center; align-items: center;">
            <q-icon style="position: absolute; z-index: 1; color: #555; font-size: 12rem; margin-top: 2rem" size="xl" name="file_upload"/>
            <h5 class="text-grey-1 text-weight-bolder" style="position: absolute; z-index: 1">Upload a Settings Configuration</h5>
            <p class="text-grey-1 text-weight-bold" style="position: absolute; z-index: 1; margin-top: 5rem">Drag and Drop Here or Click <q-icon name="add_box" size="xs" class="q-mb-xs"/> to Select a File</p>
            <q-uploader label="Upload a Settings Configuration" max-files="1" url="http://localhost:40100/settings/restore" style="height: 500px; min-width: 100%; background-image: linear-gradient(-45deg, #3d3d3d 25%, #333 25%, #333 50%, #3d3d3d 50%, #3d3d3d 75%, #333 75%, #333 100%); background-size: 100px 100px; border: dashed 1px lightgrey;"/>
        </div>
        <div class="q-mt-md">
            <p class="q-mb-none text-body1 text-weight-bold">Save Your Settings Configuration</p>
            <div class="text-caption text-grey-5">Download this domain's current settings to quickly configure another domain or to restore them later.</div>
            <q-btn color="positive" href="/settings/backup.json" target="_blank" class="q-mt-sm">Download Domain Settings</q-btn>
        </div>
    </q-card>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { Settings } from "@Modules/domain/settings";
import { AutomaticContentArchivesSaveSettings, BackupRule, SettingsValues } from "@/src/modules/domain/interfaces/settings";

export default defineComponent({
    name: "BackupRestoreSettings",

    data () {
        return {
            values: {} as SettingsValues,
            uploadedSettingsConfiguration: null
        };
    },
    methods: {
        async refreshSettingsValues (): Promise<void> {
            this.values = await Settings.getValues();
        },
        saveSettings (): void {
            const settingsToCommit: AutomaticContentArchivesSaveSettings = {
                "automatic_content_archives": {
                    "backup_rules": this.backupRules
                }
            };
            Settings.automaticCommitSettings(settingsToCommit);
        }
    },
    computed: {
        backupRules: {
            get (): BackupRule[] {
                return this.values.automatic_content_archives?.backup_rules ?? [];
            },
            set (newBackupRules: BackupRule[]): void {
                if (typeof this.values.automatic_content_archives?.backup_rules !== "undefined") {
                    this.values.automatic_content_archives.backup_rules = newBackupRules;
                    this.saveSettings();
                }
            }
        }
    },
    beforeMount () {
        this.refreshSettingsValues();
    }
});
</script>
