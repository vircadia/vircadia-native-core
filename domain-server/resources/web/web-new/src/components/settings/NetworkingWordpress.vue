<template>
    <div>
        <!-- WordPress OAuth2 Settings -->
        <q-card class="my-card q-mt-md">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold q-mb-sm">WordPress OAuth2</div>
                <q-separator />
                <!-- ADVANCED SETTINGS SECTION -->
                <q-expansion-item v-model="isWordPressSettingsToggled" class="q-mt-md text-subtitle1" popup icon="settings" label="WordPress OAuth2 Settings">
                    <q-card>
                        <!-- enable WebRTC client connections section -->
                        <q-card-section>
                            <q-toggle v-model="isOauth2AuthenticationEnabled" checked-icon="check" color="positive" label="Enable OAuth2 Authentication"
                                unchecked-icon="clear" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Allow a WordPress-based (miniOrange) OAuth2 service to assign users to groups based on their role with the service.</div>
                        </q-card-section>
                        <!-- Authentication URL section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="authenticationURL" label="Authentication URL"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">The URL that the Interface will use to login via OAuth2.</div>
                        </q-card-section>
                        <!-- WordPress API URL Base section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="wordpressAPIURL" label="WordPress API URL Base"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">The URL base that the domain server will use to make WordPress API calls. Typically "https://oursite.com/wp-json/". However, if using non-pretty permalinks or otherwise get a 404 error then try "https://oursite.com/?rest_route=/".</div>
                        </q-card-section>
                        <!-- WordPress Plugin Client ID section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="wordpressPluginID" label="WordPress Plugin Client ID"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">This is the client ID from the WordPress plugin configuration.</div>
                        </q-card-section>
                    </q-card>
                </q-expansion-item>
                <!-- *END* ADVANCED SETTINGS SECTION *END* -->
            </q-card-section>
        </q-card>
        <!-- *END* WordPress OAuth2 Settings *END* -->
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { Settings } from "@Modules/domain/settings";

export default defineComponent({
    name: "WordpressSettings",

    data () {
        return {
            isWordPressSettingsToggled: false,
            // WordPress OAuth2 section variables
            isOauth2AuthenticationEnabled: false, // TODO: get OAuth2 Authentication settings state
            authenticationURL: "example.com" as string, // TODO: get authentication URL
            wordpressAPIURL: "examplewordpressapi.com" as string, // TODO: get wordpress API URL
            wordpressPluginID: "exampleID" as string // TODO: get wordpress plugin client ID
        };
    },
    methods: {
        async refreshSettingsValues (): Promise<void> {
            const settingsValues = await Settings.getValues();
            // assigns values and checks they are not undefined
            this.isOauth2AuthenticationEnabled = settingsValues.authentication?.enable_oauth2 ?? false;
            this.authenticationURL = settingsValues.authentication?.oauth2_url_path ?? "error";
            this.wordpressAPIURL = settingsValues.authentication?.wordpress_url_base ?? "error";
            this.wordpressPluginID = settingsValues.authentication?.plugin_client_id ?? "error";
        }
    },
    beforeMount () {
        this.refreshSettingsValues();
    }
});
</script>
