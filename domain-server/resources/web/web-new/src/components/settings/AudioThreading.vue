<template>
    <div>
        <!-- Audio Threading Settings -->
        <q-card class="my-card">
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
                        <!-- Throttle Start Target section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="throttleStart" label="Throttle Start Target" type="number"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Target percentage of frame time to start throttling.</div>
                        </q-card-section>
                        <!-- Throttle Backoff Target section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="throttleBackoff" label="Throttle Backoff Target" type="number"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Target percentage of frame time to backoff throttling</div>
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
import { AudioThreadingSaveSettings, SettingsValues } from "@/src/modules/domain/interfaces/settings";

export default defineComponent({
    name: "AudioThreadingSettings",

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
                    "num_threads": this.numberOfThreads,
                    "throttle_start": this.throttleStart,
                    "throttle_backoff": this.throttleBackoff
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
        },
        throttleStart: {
            get (): number {
                return this.values.audio_threading?.throttle_start ?? 0;
            },
            set (newThrottleStart: number): void {
                if (typeof this.values.audio_threading?.throttle_start !== "undefined") {
                    this.values.audio_threading.throttle_start = newThrottleStart;
                    this.saveSettings();
                }
            }
        },
        throttleBackoff: {
            get (): number {
                return this.values.audio_threading?.throttle_backoff ?? 0;
            },
            set (newThrottleBackoff: number): void {
                if (typeof this.values.audio_threading?.throttle_backoff !== "undefined") {
                    this.values.audio_threading.throttle_backoff = newThrottleBackoff;
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
