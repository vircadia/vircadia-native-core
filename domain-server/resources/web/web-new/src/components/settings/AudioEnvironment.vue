<template>
    <div>
        <!-- Audio Environment Settings -->
        <q-card class="my-card q-ma-sm">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold q-mb-sm">Audio Environment</div>
                <q-separator />
                <!-- ADVANCED SETTINGS SECTION -->
                <q-expansion-item v-model="isWordPressSettingsToggled" class="q-mt-md text-subtitle1" popup icon="settings" label="Advanced Settings">
                    <q-card>
                        <!-- Noise Muting Threshold section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="noiseMutingThreshold" label="Noise Muting Threshold"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Loudness value for noise background between 0 and 1.0 (0: mute everyone, 1.0: never mute). 0.003 is a typical setting to mute loud people.</div>
                        </q-card-section>
                        <!-- enable Low-pass Filter section -->
                        <q-card-section>
                            <q-toggle v-model="isLowPassFilterEnabled" checked-icon="check" color="positive" label="Low-pass Filter" unchecked-icon="clear" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Positional audio stream uses low-pass filter.</div>
                        </q-card-section>
                        <!-- Audio Codec Preference Order section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="audioCodecPreferenceOrder" label="Audio Codec Preference Order"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">List of codec names in order of preferred usage.</div>
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
import type { AudioEnvSaveSettings, SettingsValues } from "@Modules/domain/interfaces/settings";

export default defineComponent({
    name: "AudioEnvironmentSettings",

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
            const settingsToCommit: AudioEnvSaveSettings = {
                "audio_env": {
                    "noise_muting_threshold": this.noiseMutingThreshold,
                    "enable_filter": this.isLowPassFilterEnabled,
                    "codec_preference_order": this.audioCodecPreferenceOrder
                }
            };
            Settings.automaticCommitSettings(settingsToCommit);
        }
    },
    computed: {
        noiseMutingThreshold: {
            get (): string {
                return this.values.audio_env?.noise_muting_threshold ?? "error";
            },
            set (newNoiseMutingThreshold: string): void {
                if (typeof this.values.audio_env?.noise_muting_threshold !== "undefined") {
                    this.values.audio_env.noise_muting_threshold = newNoiseMutingThreshold;
                    this.saveSettings();
                }
            }
        },
        isLowPassFilterEnabled: {
            get (): boolean {
                return this.values.audio_env?.enable_filter ?? false;
            },
            set (newEnableLowPassFilter: boolean): void {
                if (typeof this.values.audio_env?.enable_filter !== "undefined") {
                    this.values.audio_env.enable_filter = newEnableLowPassFilter;
                    this.saveSettings();
                }
            }
        },
        audioCodecPreferenceOrder: {
            get (): string {
                return this.values.audio_env?.codec_preference_order ?? "error";
            },
            set (newAudioCodecPreferenceOrder: string): void {
                if (typeof this.values.audio_env?.codec_preference_order !== "undefined") {
                    this.values.audio_env.codec_preference_order = newAudioCodecPreferenceOrder;
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
