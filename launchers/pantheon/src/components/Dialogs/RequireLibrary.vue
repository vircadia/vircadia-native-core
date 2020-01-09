<template>
    <v-dialog
        width="500"
    >
        <v-card>
            <v-card-title
                class="headline"
                primary-title
                dark
            >
                Notice
            </v-card-title>
    
            <v-card-text>
                The latest version of Interface is done downloading! You can install it now or press the install button in the bar below later.
            </v-card-text>
    
            <v-divider></v-divider>
    
            <v-card-actions>
                <v-spacer></v-spacer>
                <v-btn
                    color="primary"
                    text
                    @click="installInterface()"
                >
                    Install
                </v-btn>
                <v-btn
                    color="primary"
                    text
                    @click="showDownloadDone = false"
                >
                    Dismiss
                </v-btn>
            </v-card-actions>
        </v-card>
    </v-dialog>
</template>


<script>
export default {
  name: 'RequireLibrary',

  data: () => ({
    
  }),
};
</script>