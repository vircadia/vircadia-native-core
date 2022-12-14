<template>
    <div>
        <!-- Monitoring Settings -->
        <q-card class="my-card q-mt-md">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold q-mb-sm">Monitoring</div>
                <q-separator />
                <!-- ADVANCED SETTINGS SECTION -->
                <q-expansion-item v-model="isMonitoringSettingsToggled" class="q-mt-md text-subtitle1" popup icon="settings" label="Advanced Settings">
                    <q-card>
                        <!-- Enable Prometheus Exporter section -->
                        <q-card-section>
                            <q-toggle v-model="isPrometheusExporterEnabled" checked-icon="check" color="positive" label="Enable Prometheus Exporter"
                                unchecked-icon="clear" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Enable a Prometheus exporter to make it possible to gather stats about the mixers that are available in the Nodes tab with a Prometheus server. This makes it possible to keep track of long-term domain statistics for graphing, troubleshooting, and performance monitoring.</div>
                        </q-card-section>
                        <!-- Prometheus TCP Port section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="prometheusPort" label="Prometheus TCP Port"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">This is the port where the Prometheus exporter accepts connections. It listens both on IPv4 and IPv6 and can be accessed remotely, so you should make sure to restrict access with a firewall as needed.</div>
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
import { SettingsValues, MonitoringSaveSettings } from "@/src/modules/domain/interfaces/settings";

export default defineComponent({
    name: "MonitoringSettings",

    data () {
        return {
            isMonitoringSettingsToggled: false,
            values: {} as SettingsValues
        };
    },
    methods: {
        async refreshSettingsValues (): Promise<void> {
            this.values = await Settings.getValues();
        },
        saveSettings (): void {
            const settingsToCommit: MonitoringSaveSettings = {
                "monitoring": {
                    "enable_prometheus_exporter": this.isPrometheusExporterEnabled,
                    "prometheus_exporter_port": this.prometheusPort
                }
            };
            Settings.automaticCommitSettings(settingsToCommit);
        }
    },
    computed: {
        isPrometheusExporterEnabled: {
            get (): boolean {
                return this.values.monitoring?.enable_prometheus_exporter ?? false;
            },
            set (newEnablePrometheus: boolean): void {
                if (typeof this.values.monitoring?.enable_prometheus_exporter !== "undefined") {
                    this.values.monitoring.enable_prometheus_exporter = newEnablePrometheus;
                    this.saveSettings();
                }
            }
        },
        prometheusPort: {
            get (): string {
                return this.values.monitoring?.prometheus_exporter_port ?? "error";
            },
            set (newPrometheusPort: string): void {
                if (typeof this.values.monitoring?.prometheus_exporter_port !== "undefined") {
                    this.values.monitoring.prometheus_exporter_port = newPrometheusPort;
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
