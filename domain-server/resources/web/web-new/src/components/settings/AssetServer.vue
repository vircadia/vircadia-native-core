<template>
    <div>
        <!--  Asset Server (ATP) Settings -->
        <q-card class="my-card q-ma-sm">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold q-mb-sm">Asset Server (ATP)</div>
                <q-separator />
                <!-- ADVANCED SETTINGS SECTION -->
                <q-expansion-item v-model="isWordPressSettingsToggled" class="q-mt-md text-subtitle1" popup icon="settings" label="Advanced Settings">
                    <q-card>
                        <!-- Enable Asset Server section -->
                        <q-card-section>
                            <q-toggle v-model="assetServerEnabled" checked-icon="check" color="positive" label="Enable Asset Server" unchecked-icon="clear" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Assigns an asset-server in your domain to serve files to clients via the ATP protocol (over UDP).</div>
                        </q-card-section>
                        <!-- Assets Path section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="assetsPath" label="Assets Path"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">The path to the directory assets are stored in. If this path is relative, it will be relative to the application data directory. If you change this path you will need to manually copy any existing assets from the previous directory.</div>
                        </q-card-section>
                        <!-- File Size Limit section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="fileSizeLimit" label="File Size Limit"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">The file size limit of an asset that can be imported into the asset server in MBytes. 0 (default) means no limit on file size.</div>
                        </q-card-section>
                    </q-card>
                </q-expansion-item>
                <!-- *END* ADVANCED SETTINGS SECTION *END* -->
            </q-card-section>
        </q-card>
        <!-- *END* Asset Server (ATP) Settings *END* -->
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { Settings } from "@Modules/domain/settings";
import { AssetServerSaveSettings, SettingsValues } from "@/src/modules/domain/interfaces/settings";

export default defineComponent({
    name: "AssetServerSettings",

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
            const settingsToCommit: AssetServerSaveSettings = {
                "asset_server": {
                    "enabled": this.assetServerEnabled,
                    "assets_path": this.assetsPath,
                    "assets_filesize_limit": this.fileSizeLimit
                }
            };
            Settings.automaticCommitSettings(settingsToCommit);
        }
    },
    computed: {
        assetServerEnabled: {
            get (): boolean {
                return this.values.asset_server?.enabled ?? false;
            },
            set (newAssetServerEnabled: boolean): void {
                if (typeof this.values.asset_server?.enabled !== "undefined") {
                    this.values.asset_server.enabled = newAssetServerEnabled;
                    this.saveSettings();
                }
            }
        },
        assetsPath: {
            get (): string {
                return this.values.asset_server?.assets_path ?? "error";
            },
            set (newAssetsPath: string): void {
                if (typeof this.values.asset_server?.assets_path !== "undefined") {
                    this.values.asset_server.assets_path = newAssetsPath;
                    this.saveSettings();
                }
            }
        },
        fileSizeLimit: {
            get (): number {
                return this.values.asset_server?.assets_filesize_limit ?? 0;
            },
            set (newFileSizeLimit: number): void {
                if (typeof this.values.asset_server?.assets_filesize_limit !== "undefined") {
                    this.values.asset_server.assets_filesize_limit = newFileSizeLimit;
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
