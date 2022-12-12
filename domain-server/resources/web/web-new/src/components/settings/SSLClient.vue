<template>
    <div>
        <!-- SSL ACME Client Settings -->
        <q-card class="my-card">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold q-mb-sm">SSL ACME Client</div>
                <q-separator />
                <!-- ADVANCED SETTINGS SECTION -->
                <q-expansion-item v-model="isSSLClientSettingsToggled" class="q-mt-md text-subtitle1" popup icon="settings" label="Advanced Settings">
                    <q-card>
                        <!-- enable SSL ACME Client section -->
                        <q-card-section>
                            <q-toggle v-model="isSSLClientEnabled" checked-icon="check" color="positive" label="Enable ACME Client"
                                unchecked-icon="clear" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Enables ACME client that will manage the SSL certificates.</div>
                        </q-card-section>
                    </q-card>
                </q-expansion-item>
                <!-- *END* ADVANCED SETTINGS SECTION *END* -->
            </q-card-section>
        </q-card>
        <!-- *END* WebRTC Settings *END* -->
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { Settings } from "@Modules/domain/settings";
import { SettingsValues, WebrtcSaveSettings } from "@/src/modules/domain/interfaces/settings";

export default defineComponent({
    name: "SSLClientSettings",

    data () {
        return {
            isSSLClientSettingsToggled: false,
            values: {} as SettingsValues
        };
    },
    methods: {
        async refreshSettingsValues (): Promise<void> {
            this.values = await Settings.getValues();
        },
        saveSettings (): void {
            const settingsToCommit: WebrtcSaveSettings = {
            };
            Settings.automaticCommitSettings(settingsToCommit);
        }
    },
    computed: {
        isSSLClientEnabled: {
            get (): boolean {
                return this.values.webrtc?.enable_webrtc ?? false;
            },
            set (newEnableWebrtc: boolean): void {
                if (typeof this.values.webrtc?.enable_webrtc !== "undefined") {
                    this.values.webrtc.enable_webrtc = newEnableWebrtc;
                    this.saveSettings();
                }
            }
        },
        isWebsocketSSLEnabled: {
            get (): boolean {
                return this.values.webrtc?.enable_webrtc_websocket_ssl ?? false;
            },
            set (newEnableWebsocketSSL: boolean): void {
                if (typeof this.values.webrtc?.enable_webrtc_websocket_ssl !== "undefined") {
                    this.values.webrtc.enable_webrtc_websocket_ssl = newEnableWebsocketSSL;
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
