<template>
    <div>
        <!-- Messages Mixer Settings -->
        <q-card class="my-card q-ma-sm">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold q-mb-sm">Messages Mixer</div>
                <q-separator />
                <!-- ADVANCED SETTINGS SECTION -->
                <q-expansion-item v-model="isWordPressSettingsToggled" class="q-mt-md text-subtitle1" popup icon="settings" label="Advanced Settings">
                    <q-card>
                        <!-- Maximum Message Rate section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="maxNodeMessagesPerSecond" label="Maximum Message Rate"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Maximum message send rate (messages per second) per node.</div>
                        </q-card-section>
                    </q-card>
                </q-expansion-item>
                <!-- *END* ADVANCED SETTINGS SECTION *END* -->
            </q-card-section>
        </q-card>
        <!-- *END* Messages Mixer Settings *END* -->
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { Settings } from "@Modules/domain/settings";
import type { MessagesMixerSaveSettings, SettingsValues } from "@Modules/domain/interfaces/settings";

export default defineComponent({
    name: "MessagesMixerSettings",
    data () {
        return {
            isWordPressSettingsToggled: false,
            values: {} as SettingsValues
        };
    },
    methods: {
        async refreshSettingsValues (): Promise<void> {
            this.values = await Settings.getValues();
        },
        saveSettings (): void {
            const settingsToCommit: MessagesMixerSaveSettings = {
                "messages_mixer": {
                    "max_node_messages_per_second": this.maxNodeMessagesPerSecond
                }
            };
            Settings.automaticCommitSettings(settingsToCommit);
        }
    },
    computed: {
        maxNodeMessagesPerSecond: {
            get (): number {
                return this.values.messages_mixer?.max_node_messages_per_second ?? 0;
            },
            set (newMaxNodeMessagesPerSecond: number): void {
                if (typeof this.values.messages_mixer?.max_node_messages_per_second !== "undefined") {
                    this.values.messages_mixer.max_node_messages_per_second = newMaxNodeMessagesPerSecond;
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
