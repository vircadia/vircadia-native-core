<template>
    <div>
        <!-- WebRTC Settings -->
        <q-card class="my-card q-mt-md">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold q-mb-sm">WebRTC</div>
                <q-separator />
                <!-- ADVANCED SETTINGS SECTION -->
                <q-expansion-item v-model="isWebRTCSettingsToggled" class="q-mt-md text-subtitle1" popup icon="settings" label="Advanced Settings">
                    <q-card>
                        <!-- enable WebRTC client connections section -->
                        <q-card-section>
                            <q-toggle v-model="isWebRTCConnectionsEnabled" checked-icon="check" color="positive" label="Enable WebRTC Client Connections"
                                unchecked-icon="clear" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Allow web clients to connect over WebRTC data channels.</div>
                        </q-card-section>
                        <!-- enable WebRTC WebSocket SSL section -->
                        <q-card-section>
                            <q-toggle v-model="isWebsocketSSLEnabled" checked-icon="check" color="positive" label="Enable WebRTC WebSocket SSL"
                                unchecked-icon="clear" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Use secure WebSocket (wss:// protocol) for WebRTC signaling channel. If "on", the key, cert, and CA files are expected to be in the local Vircadia app directory, in a /domain-server/ subdirectory with filenames vircadia-cert.key, vircadia-cert.crt, and vircadia-crt-ca.crt.</div>
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

export default defineComponent({
    name: "WebRTCSettings",

    data () {
        return {
            isWebRTCSettingsToggled: false,
            // WebRTC section variables
            isWebRTCConnectionsEnabled: false, // TODO: get web RTC client connections settings state
            isWebsocketSSLEnabled: false // TODO: get webRTC webSocket SSL settings state
        };
    },
    methods: {
        async refreshSettingsValues (): Promise<void> {
            const settingsValues = await Settings.getValues();
            // assigns values and checks they are not undefined
            this.isWebRTCConnectionsEnabled = settingsValues.webrtc?.enable_webrtc ?? false;
            this.isWebsocketSSLEnabled = settingsValues.webrtc?.enable_webrtc_websocket_ssl ?? false;
        }
    },
    beforeMount () {
        this.refreshSettingsValues();
    }
});
</script>
