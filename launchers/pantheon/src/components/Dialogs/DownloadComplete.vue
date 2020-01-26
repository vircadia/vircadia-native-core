<template>
    <v-dialog
        width="500"
        persistent
        v-model="showDownloadComplete"
    >
        <v-card>
            <v-card-title
                class="headline"
                primary-title
                dark
            >
                <v-icon color="success" class="mr-2">mdi-check</v-icon>
                Download Complete
            </v-card-title>
    
            <v-card-text>
                The latest version of Interface is done downloading! You can install it by pressing the "Install Interface" button.
            </v-card-text>
    
            <v-divider></v-divider>
    
            <v-card-actions>
                <v-spacer></v-spacer>
                <v-btn
                    color="primary"
                    text
                    @click="$emit('hideDialog')"
                >
                    Dismiss
                </v-btn>
            </v-card-actions>
        </v-card>
    </v-dialog>
</template>


<script>
export default {
  name: 'DownloadComplete',

  data: () => ({
      showDownloadComplete: true,
  }),
};
</script>