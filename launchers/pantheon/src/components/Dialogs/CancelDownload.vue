<template>
    <v-dialog
        width="500"
        persistent
        v-model="showCancelDownload"
    >
        <v-card>
            <v-card-title
                class="headline"
                primary-title
                dark
            >
                <v-icon color="green" class="mr-2">mdi-comment-question</v-icon>
                Cancel Download?
            </v-card-title>
    
            <v-card-text>
                Do you want to cancel the download?
            </v-card-text>
    
            <v-divider></v-divider>
    
            <v-card-actions>
                <v-spacer></v-spacer>
                <v-btn
                    color="primary"
                    text
                    @click="cancelDownload(); $emit('hideDialog')"
                >
                    Yes
                </v-btn>
                <v-btn
                    color="primary"
                    text
                    @click="continueDownload(); $emit('hideDialog')"
                >
                    No
                </v-btn>
            </v-card-actions>
        </v-card>
    </v-dialog>
</template>


<script>
const { ipcRenderer } = require('electron');

export default {
    name: 'DownloadFailed',
	methods: {
        continueDownload: function () {
            // Nothing to do/
        },
        cancelDownload: function () {
            ipcRenderer.send('cancel-download');
        },
     },
    data: () => ({
        showCancelDownload: true,
    }),
};
</script>