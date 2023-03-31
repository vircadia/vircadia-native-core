<template>
    <!-- Avatar Mixer Settings -->
    <q-card class="my-card q-ma-sm">
        <q-card-section>
            <div class="text-h5 text-center text-weight-bold q-mb-sm">Avatar Mixer</div>
            <q-separator />
            <!-- ADVANCED SETTINGS SECTION -->
            <q-expansion-item v-model="isWordPressSettingsToggled" class="q-mt-md text-subtitle1" popup icon="settings" label="Advanced Settings">
                <q-card>
                    <!-- Per-Node Bandwidth section -->
                    <q-card-section>
                        <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="maxNodeSendBandwidth" label="Per-Node Bandwidth"/>
                        <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Desired maximum send bandwidth (in Megabits per second) to each node.</div>
                    </q-card-section>
                    <!-- Automatically determine thread count section -->
                    <q-card-section>
                        <q-toggle v-model="isAutomaticThreadCountEnabled" checked-icon="check" color="positive" label="Automatically Determine Thread Count" unchecked-icon="clear" />
                        <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Allow system to determine number of threads (recommended).</div>
                    </q-card-section>
                    <!-- Number of Threads section -->
                    <q-card-section>
                        <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="numberOfThreads" label="Number of Threads"/>
                        <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Threads to spin up for avatar mixing (if not automatically set).</div>
                    </q-card-section>
                    <!-- Connection Rate section -->
                    <q-card-section>
                        <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="connectionRate" label="Connection Rate"/>
                        <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Number of new agents that can connect to the mixer every second.</div>
                    </q-card-section>
                    <!-- Hero Bandwidth section -->
                    <q-card-section>
                        <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="heroBandwidth" label="Hero Bandwidth"/>
                        <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Fraction of downstream bandwidth reserved for avatars in 'Hero' zones.</div>
                    </q-card-section>
                </q-card>
            </q-expansion-item>
            <!-- *END* ADVANCED SETTINGS SECTION *END* -->
        </q-card-section>
    </q-card>
    <!-- *END* Avatar Mixer Settings *END* -->
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { Settings } from "@Modules/domain/settings";
import type { AvatarMixerSaveSettings, SettingsValues } from "@Modules/domain/interfaces/settings";

export default defineComponent({
    name: "AvatarMixerSettings",
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
            const settingsToCommit: AvatarMixerSaveSettings = {
                "avatar_mixer": {
                    "max_node_send_bandwidth": this.maxNodeSendBandwidth,
                    "auto_threads": this.isAutomaticThreadCountEnabled,
                    "num_threads": this.numberOfThreads,
                    "connection_rate": this.connectionRate,
                    "priority_fraction": this.heroBandwidth
                }
            };
            Settings.automaticCommitSettings(settingsToCommit);
        }
    },
    computed: {
        maxNodeSendBandwidth: {
            get (): number {
                return this.values.avatar_mixer?.max_node_send_bandwidth ?? 5;
            },
            set (newMaxNodeSendBandwidth: number): void {
                if (typeof this.values.avatar_mixer?.max_node_send_bandwidth !== "undefined") {
                    this.values.avatar_mixer.max_node_send_bandwidth = newMaxNodeSendBandwidth;
                    this.saveSettings();
                }
            }
        },
        isAutomaticThreadCountEnabled: {
            get (): boolean {
                return this.values.avatar_mixer?.auto_threads ?? true;
            },
            set (newAutomaticThreadCount: boolean): void {
                if (typeof this.values.avatar_mixer?.auto_threads !== "undefined") {
                    this.values.avatar_mixer.auto_threads = newAutomaticThreadCount;
                    this.saveSettings();
                }
            }
        },
        numberOfThreads: {
            get (): string {
                return this.values.avatar_mixer?.num_threads ?? "error";
            },
            set (newNumberOfThreads: string): void {
                if (typeof this.values.avatar_mixer?.num_threads !== "undefined") {
                    this.values.avatar_mixer.num_threads = newNumberOfThreads;
                    this.saveSettings();
                }
            }
        },
        connectionRate: {
            get (): string {
                return this.values.avatar_mixer?.connection_rate ?? "error";
            },
            set (newConnectionRate: string): void {
                if (typeof this.values.avatar_mixer?.connection_rate !== "undefined") {
                    this.values.avatar_mixer.connection_rate = newConnectionRate;
                    this.saveSettings();
                }
            }
        },
        heroBandwidth: {
            get (): string {
                return this.values.avatar_mixer?.priority_fraction ?? "error";
            },
            set (newHeroBandwidth: string): void {
                if (typeof this.values.avatar_mixer?.priority_fraction !== "undefined") {
                    this.values.avatar_mixer.priority_fraction = newHeroBandwidth;
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
