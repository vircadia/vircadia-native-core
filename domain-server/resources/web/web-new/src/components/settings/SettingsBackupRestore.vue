<template>
    <q-card class="my-card q-pa-md">
        <p class="q-mb-xs text-body1 text-weight-bold">Upload a settings configuration to quickly configure this domain</p>
        <div class="text-caption text-warning">Note: Your domain settings will be replaced by the settings you upload.</div>
        <div style="display: flex; justify-content: center; align-items: center;">
            <q-icon style="position: absolute; z-index: 1; color: #555; font-size: 12rem; margin-top: 2rem" size="xl" name="file_upload"/>
            <h5 class="text-grey-1 text-weight-bolder" style="position: absolute; z-index: 1">Upload a Settings Configuration</h5>
            <p class="text-grey-1 text-weight-bold" style="position: absolute; z-index: 1; margin-top: 5rem">Drag and Drop Here or Click <q-icon name="add_box" size="xs" class="q-mb-xs"/>to Select a File</p>
            <q-uploader label="Upload a Settings Configuration" max-files="1" url="/settings/restore" @uploaded="successfulUpload" @failed="failedUpload" style="height: 500px; min-width: 100%; background-image: linear-gradient(-45deg, #3d3d3d 25%, #333 25%, #333 50%, #3d3d3d 50%, #3d3d3d 75%, #333 75%, #333 100%); background-size: 100px 100px; border: dashed 1px lightgrey;"/>
        </div>
        <div class="q-mt-md">
            <p class="q-mb-none text-body1 text-weight-bold">Save Your Settings Configuration</p>
            <div class="text-caption text-grey-5">Download this domain's current settings to quickly configure another domain or to restore them later.</div>
            <q-btn color="positive" href="/settings/backup.json" target="_blank" class="q-mt-sm">Download Domain Settings</q-btn>
        </div>
        <q-dialog v-model="showSuccessfulUploadDialog" persistent>
            <q-card class="bg-positive q-pa-md">
                <q-card-section class="row items-center">
                    <span class="text-h5 q-mb-sm text-bold text-white">Upload Successful!</span>
                    <span class="text-body2">Your settings will be commmited and the page will refresh in <span class="text-bold">{{ reloadPageTimer }}</span> seconds</span>
                </q-card-section>
            </q-card>
        </q-dialog>
        <q-dialog v-model="showFailedUploadDialog" auto-close>
            <q-card class="bg-negative q-pa-md">
                <q-card-section class="row items-center">
                    <span class="text-h5 q-mb-sm text-bold text-white">Upload Failed</span>
                    <span class="text-body2">Please try again or upload a different settings configuration file.</span>
                </q-card-section>
            </q-card>
        </q-dialog>
    </q-card>
</template>

<script lang="ts">
import { defineComponent } from "vue";

export default defineComponent({
    name: "BackupRestoreSettings",

    data () {
        return {
            uploadedSettingsConfiguration: null,
            showSuccessfulUploadDialog: false,
            reloadPageTimer: 3,
            showFailedUploadDialog: false
        };
    },
    methods: {
        successfulUpload () {
            this.showSuccessfulUploadDialog = true;
            window.setInterval(() => {
                this.reloadPageTimer--;
                if (this.reloadPageTimer === 0) {
                    window.location.reload();
                }
            }, 1000);
        },
        failedUpload () {
            console.log("Failed to upload settings configuration");
            this.showFailedUploadDialog = true;
        }
    }
});
</script>
