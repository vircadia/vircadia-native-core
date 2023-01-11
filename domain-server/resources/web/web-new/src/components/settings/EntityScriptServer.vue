<template>
    <div>
        <!-- Entities Script Server Settings -->
        <q-card class="my-card q-mt-md">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold q-mb-sm">Audio Threading</div>
                <q-separator />
                <!-- ADVANCED SETTINGS SECTION -->
                <q-expansion-item v-model="isWordPressSettingsToggled" class="q-mt-md text-subtitle1" popup icon="settings" label="Advanced Settings">
                    <q-card>
                        <!-- enable automatic thread count section -->
                        <q-card-section>
                            <q-toggle v-model="isAutomaticThreadCountEnabled" checked-icon="check" color="positive" label="Automatically Determine Thread Count" unchecked-icon="clear" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Allow system to determine number of threads (recommended).</div>
                        </q-card-section>
                        <!-- Number of Threads section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="numberOfThreads" label="Number of Threads"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Threads to spin up for audio mixing (if not automatically set).</div>
                        </q-card-section>
                    </q-card>
                </q-expansion-item>
                <!-- *END* ADVANCED SETTINGS SECTION *END* -->
            </q-card-section>
        </q-card>
        <!-- *END* Entities Settings *END* -->
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { Settings } from "@Modules/domain/settings";
import { AudioThreadingSaveSettings, SettingsValues } from "@/src/modules/domain/interfaces/settings";

export default defineComponent({
    name: "EntityScriptServerSettings",

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
            const settingsToCommit: AudioThreadingSaveSettings = {
                "audio_threading": {
                    "auto_threads": this.isAutomaticThreadCountEnabled,
                    "num_threads": this.numberOfThreads
                }
            };
            Settings.automaticCommitSettings(settingsToCommit);
        }
    },
    computed: {
        isAutomaticThreadCountEnabled: {
            get (): boolean {
                return this.values.audio_threading?.auto_threads ?? false;
            },
            set (newEnableAutoThreads: boolean): void {
                if (typeof this.values.audio_threading?.auto_threads !== "undefined") {
                    this.values.audio_threading.auto_threads = newEnableAutoThreads;
                    this.saveSettings();
                }
            }
        },
        numberOfThreads: {
            get (): string {
                return this.values.audio_threading?.num_threads ?? "error";
            },
            set (newNumberOfThreads: string): void {
                if (typeof this.values.audio_threading?.num_threads !== "undefined") {
                    this.values.audio_threading.num_threads = newNumberOfThreads;
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
