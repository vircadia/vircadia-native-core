<template>
    <div>
        <!-- Upload Content -->
        <q-card class="my-card q-mt-md">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold q-mb-sm">Upload Content</div>
                <q-separator />
                <q-card>
                    <q-card-section>
                        <p class="q-mb-xs text-body1 text-weight-bold">Upload Content</p>
                        <div class="q-mt-xs text-caption text-grey-5">Upload a content archive (.zip) or entity file (.json, .json.gz) to replace the content of this domain.<br/>
                        <span class="text-warning">Note: Your domain content will be replaced by the content you upload, but the existing backup files of your domain's content will not immediately be changed.</span></div>
                        <div style="display: flex; justify-content: center; align-items: center;">
                            <q-icon style="position: absolute; z-index: 1; color: #555; font-size: 12rem; margin-top: 2rem" size="xl" name="file_upload"/>
                            <h5 class="text-grey-1 text-weight-bolder" style="position: absolute; z-index: 1">Upload Content</h5>
                            <p class="text-grey-1 text-weight-bold" style="position: absolute; z-index: 1; margin-top: 5rem">Drag and Drop Here or Click <q-icon name="add_box" size="xs" class="q-mb-xs"/> to Select a File</p>
                            <q-uploader label="Upload a content archive (.zip) or entity file (.json .json.gz)" max-files="1" url="/content/upload" style="height: 500px; min-width: 100%; background-image: linear-gradient(-45deg, #3d3d3d 25%, #333 25%, #333 50%, #3d3d3d 50%, #3d3d3d 75%, #333 75%, #333 100%); background-size: 100px 100px; border: dashed 1px lightgrey;"/>
                        </div>
                        <q-dialog v-model="showSuccessfulUploadDialog" persistent>
                            <q-card class="bg-positive q-pa-md">
                                <q-card-section class="row items-center">
                                    <span class="text-h5 q-mb-sm text-bold text-white">Upload Successful!</span>
                                    <span class="text-body2">Your uploaded content will be saved and the page will refresh in <span class="text-bold">{{ reloadPageTimer }}</span> seconds</span>
                                </q-card-section>
                            </q-card>
                        </q-dialog>
                        <q-dialog v-model="showFailedUploadDialog" auto-close>
                            <q-card class="bg-negative q-pa-md">
                                <q-card-section class="row items-center">
                                    <span class="text-h5 q-mb-sm text-bold text-white">Upload Failed</span>
                                    <span class="text-body2">Please try again or upload a different content archive/file from another backup.</span>
                                </q-card-section>
                            </q-card>
                        </q-dialog>
                    </q-card-section>
                </q-card>
            </q-card-section>
        </q-card>
        <!-- *END* Content Archives *END* -->
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";

export default defineComponent({
    name: "UploadContent",
    data () {
        return {
            showSuccessfulUploadDialog: false,
            reloadPageTimer: 3,
            showFailedUploadDialog: false
        };
    },
    methods: {
        successfulUpload (): void {
            this.showSuccessfulUploadDialog = true;
            window.setInterval(() => {
                this.reloadPageTimer--;
                if (this.reloadPageTimer === 0) {
                    window.location.reload();
                }
            }, 1000);
        },
        failedUpload (): void {
            console.log("Failed to upload content");
            this.showFailedUploadDialog = true;
        }
    }
});
</script>
