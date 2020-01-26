<template>
    <v-dialog
        width="500"
        persistent
        v-model="showNoInstallerFound"
    >
        <v-card>
            <v-card-title
                class="headline"
                primary-title
                dark
            >
                <v-icon color="red" class="mr-2">mdi-message-alert</v-icon>
                No Installer Found
            </v-card-title>
    
            <v-card-text>
                An installer for interface was not found. Try downloading it by pressing the "Download Interface" button.
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
    name: 'NoInstallerFound',

    data: () => ({
        showNoInstallerFound: true,
    }),
    created: function () {
        console.info("What?", this.showNoInstallerFound);
    },
};
</script>