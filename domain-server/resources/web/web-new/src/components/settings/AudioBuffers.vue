<template>
    <div>
        <!-- Audio Buffers Settings -->
        <q-card class="my-card q-ma-sm">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold q-mb-sm">Audio Buffers</div>
                <q-separator />
                <!-- ADVANCED SETTINGS SECTION -->
                <q-expansion-item v-model="isWordPressSettingsToggled" class="q-mt-md text-subtitle1" popup icon="settings" label="Advanced Settings">
                    <q-card>
                        <!-- Dynamic Jitter Buffers section -->
                        <q-card-section>
                            <q-toggle v-model="dyanmicJitterBuffers" checked-icon="check" color="positive" label="Dynamic Jitter Buffers" unchecked-icon="clear" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Dynamically buffer inbound audio streams based on perceived jitter in packet receipt timing.</div>
                        </q-card-section>
                        <!-- Static Desired Jitter Buffer Frames section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="staticDesiredJitterBufferFrames" label="Static Desired Jitter Buffer Frames"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">If dynamic jitter buffers is disabled, this determines the size of the jitter buffers of inbound audio streams in the mixer. Higher numbers introduce more latency.</div>
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
import type { AudioBufferSaveSettings, SettingsValues } from "@Modules/domain/interfaces/settings";

export default defineComponent({
    name: "AudioBuffersSettings",

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
            const settingsToCommit: AudioBufferSaveSettings = {
                "audio_buffer": {
                    "dynamic_jitter_buffer": this.dyanmicJitterBuffers,
                    "static_desired_jitter_buffer_frames": this.staticDesiredJitterBufferFrames
                }
            };
            Settings.automaticCommitSettings(settingsToCommit);
        }
    },
    computed: {
        dyanmicJitterBuffers: {
            get (): boolean {
                return this.values.audio_buffer?.dynamic_jitter_buffer ?? false;
            },
            set (newEnableJitterBuffer: boolean): void {
                if (typeof this.values.audio_buffer?.dynamic_jitter_buffer !== "undefined") {
                    this.values.audio_buffer.dynamic_jitter_buffer = newEnableJitterBuffer;
                    this.saveSettings();
                }
            }
        },
        staticDesiredJitterBufferFrames: {
            get (): string {
                return this.values.audio_buffer?.static_desired_jitter_buffer_frames ?? "error";
            },
            set (newStaticDesiredJitterBufferFrames: string): void {
                if (typeof this.values.audio_buffer?.static_desired_jitter_buffer_frames !== "undefined") {
                    this.values.audio_buffer.static_desired_jitter_buffer_frames = newStaticDesiredJitterBufferFrames;
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
