<template>
    <div>
        <!-- Entities Settings -->
        <q-card class="my-card q-ma-sm">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold q-mb-sm">Entities</div>
                <q-separator />
                <!-- ADVANCED SETTINGS SECTION -->
                <q-expansion-item v-model="isWordPressSettingsToggled" class="q-mt-md text-subtitle1" popup icon="settings" label="Advanced Settings">
                    <q-card>
                        <!-- Maximum Lifetime of Temporary Entities section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="maxLifetimeOfTemporaryEntities" label="Maximum Lifetime of Temporary Entities"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">The maximum number of seconds for the lifetime of an entity which will be considered "temporary".</div>
                        </q-card-section>
                        <!-- Dynamic Domain Verification Time (seconds) - Minimum section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="minDomainVertificationTime" label="Dynamic Domain Verification Time (seconds) - Minimum"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">The lower limit on the amount of time that passes before Dynamic Domain Verification on entities occurs. Units are seconds.</div>
                        </q-card-section>
                        <!-- Dynamic Domain Verification Time (seconds) - Maximum section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="maxDomainVertificationTime" label="Dynamic Domain Verification Time (seconds) - Maximum"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">The upper limit on the amount of time that passes before Dynamic Domain Verification on entities occurs. Units are seconds.</div>
                        </q-card-section>
                        <!-- Entity Scripts Allowed from: section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="scriptSourceWhitelist" label="Entity Scripts Allowed from:"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Comma separated list of URLs (with optional paths) that entity scripts are allowed from. If someone attempts to create and entity or edit an entity to have a different domain, it will be rejected. If left blank, any domain is allowed.</div>
                        </q-card-section>
                        <!-- Entities File Path section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="entityFilePath" label="Entities File Path"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">The path to the file entities are stored in. If this path is relative it will be relative to the application data directory. The filename must end in .json.gz.</div>
                        </q-card-section>
                        <!-- Entities Backup Directory Path section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="backupDirectoryPath" label="Entities Backup Directory Path"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">The path to the directory to store backups in. If this path is relative it will be relative to the application data directory.</div>
                        </q-card-section>
                        <!-- Save Check Interval section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="saveCheckInterval" label="Save Check Interval"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Milliseconds between checks for saving the current state of entities.</div>
                        </q-card-section>
                        <!-- Disable Persistence section -->
                        <q-card-section>
                            <q-toggle v-model="isNoPersistEnabled" checked-icon="check" color="positive" label="Disable Persistence" unchecked-icon="clear" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Don't persist your entities to a file.</div>
                        </q-card-section>
                        <!-- Disable Backup section -->
                        <q-card-section>
                            <q-toggle v-model="isNoBackupEnabled" checked-icon="check" color="positive" label="Disable Backup" unchecked-icon="clear" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Don't regularly backup your persisted entities to a backup file.</div>
                        </q-card-section>
                        <!-- Status Hostname section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="statusHost" label="Status Hostname"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Host name or IP address of the server for accessing the status page.</div>
                        </q-card-section>
                        <!-- Status Port section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="statusPort" label="Status Port"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Port of the server for accessing the status page.</div>
                        </q-card-section>
                        <!-- Persist File Download section -->
                        <q-card-section>
                            <q-toggle v-model="isPersistFileDownloadEnabled" checked-icon="check" color="positive" label="Persist File Download" unchecked-icon="clear" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Includes a download link to the persist file in the server status page.</div>
                        </q-card-section>
                        <!-- Edit Logging section -->
                        <q-card-section>
                            <q-toggle v-model="isEditLoggingEnabled" checked-icon="check" color="positive" label="Edit Logging" unchecked-icon="clear" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Logging of all edits to entities.</div>
                        </q-card-section>
                        <!-- Edit Logging (Terse) section -->
                        <q-card-section>
                            <q-toggle v-model="isTerseEditLoggingEnabled" checked-icon="check" color="positive" label="Edit Logging (Terse)" unchecked-icon="clear" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Logging of all edits to entities.</div>
                        </q-card-section>
                        <!-- Verbose Debug section -->
                        <q-card-section>
                            <q-toggle v-model="isVerboseDebugEnabled" checked-icon="check" color="positive" label="Verbose Debug" unchecked-icon="clear" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Lots of debugging.</div>
                        </q-card-section>
                        <!-- Extra Receiving Debug section -->
                        <q-card-section>
                            <q-toggle v-model="isReceivingDebugEnabled" checked-icon="check" color="positive" label="Extra Receiving Debug" unchecked-icon="clear" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Extra debugging on receiving.</div>
                        </q-card-section>
                        <!-- Extra Sending Debug section -->
                        <q-card-section>
                            <q-toggle v-model="isSendingDebugEnabled" checked-icon="check" color="positive" label="Extra Sending Debug" unchecked-icon="clear" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Extra debugging on sending.</div>
                        </q-card-section>
                        <!-- Extra Timestamp Debugging section -->
                        <q-card-section>
                            <q-toggle v-model="isTimestampDebugEnabled" checked-icon="check" color="positive" label="Extra Timestamp Debugging" unchecked-icon="clear" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Extra debugging for usecTimestampNow() function.</div>
                        </q-card-section>
                        <!-- Clock Skew section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="clockSkew" label="Clock Skew"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Number of msecs to skew the server clock by to test clock skew.</div>
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
import type { EntityServerSaveSettings, SettingsValues } from "@Modules/domain/interfaces/settings";

export default defineComponent({
    name: "EntitiesSettings",

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
            const settingsToCommit: EntityServerSaveSettings = {
                "entity_server_settings": {
                    "maxTmpLifetime": this.maxLifetimeOfTemporaryEntities,
                    "dynamicDomainVerificationTimeMin": this.minDomainVertificationTime,
                    "dynamicDomainVerificationTimeMax": this.maxDomainVertificationTime,
                    "entityScriptSourceWhitelist": this.scriptSourceWhitelist,
                    "persistFilePath": this.entityFilePath,
                    "persistInterval": this.saveCheckInterval,
                    "NoPersist": this.isNoPersistEnabled,
                    "NoBackup": this.isNoBackupEnabled,
                    "statusHost": this.statusHost,
                    "statusPort": this.statusPort,
                    "persistFileDownload": this.isPersistFileDownloadEnabled,
                    "wantEditLogging": this.isEditLoggingEnabled,
                    "wantTerseEditLogging": this.isTerseEditLoggingEnabled,
                    "verboseDebug": this.isVerboseDebugEnabled,
                    "debugReceiving": this.isReceivingDebugEnabled,
                    "debugSending": this.isSendingDebugEnabled,
                    "debugTimestampNow": this.isTimestampDebugEnabled,
                    "clockSkew": this.clockSkew
                }
            };
            Settings.automaticCommitSettings(settingsToCommit);
        }
    },
    computed: {
        maxLifetimeOfTemporaryEntities: {
            get (): string {
                return this.values.entity_server_settings?.maxTmpLifetime ?? "error";
            },
            set (newMaxLifetime: string): void {
                if (typeof this.values.entity_server_settings?.maxTmpLifetime !== "undefined") {
                    this.values.entity_server_settings.maxTmpLifetime = newMaxLifetime;
                    this.saveSettings();
                }
            }
        },
        minDomainVertificationTime: {
            get (): string {
                return this.values.entity_server_settings?.dynamicDomainVerificationTimeMin ?? "error";
            },
            set (newMinDomainVertificationTime: string): void {
                if (typeof this.values.entity_server_settings?.dynamicDomainVerificationTimeMin !== "undefined") {
                    this.values.entity_server_settings.dynamicDomainVerificationTimeMin = newMinDomainVertificationTime;
                    this.saveSettings();
                }
            }
        },
        maxDomainVertificationTime: {
            get (): string {
                return this.values.entity_server_settings?.dynamicDomainVerificationTimeMax ?? "error";
            },
            set (newMaxDomainVertificationTime: string): void {
                if (typeof this.values.entity_server_settings?.dynamicDomainVerificationTimeMax !== "undefined") {
                    this.values.entity_server_settings.dynamicDomainVerificationTimeMax = newMaxDomainVertificationTime;
                    this.saveSettings();
                }
            }
        },
        scriptSourceWhitelist: {
            get (): string {
                return this.values.entity_server_settings?.entityScriptSourceWhitelist ?? "error";
            },
            set (newScriptSourceWhitelist: string): void {
                if (typeof this.values.entity_server_settings?.entityScriptSourceWhitelist !== "undefined") {
                    this.values.entity_server_settings.entityScriptSourceWhitelist = newScriptSourceWhitelist;
                    this.saveSettings();
                }
            }
        },
        entityFilePath: {
            get (): string {
                return this.values.entity_server_settings?.persistFilePath ?? "error";
            },
            set (newEntityFilePath: string): void {
                if (typeof this.values.entity_server_settings?.persistFilePath !== "undefined") {
                    this.values.entity_server_settings.persistFilePath = newEntityFilePath;
                    this.saveSettings();
                }
            }
        },
        backupDirectoryPath: {
            get (): string {
                return this.values.entity_server_settings?.backupDirectoryPath ?? "error";
            },
            set (newBackupDirectoryPath: string): void {
                if (typeof this.values.entity_server_settings?.backupDirectoryPath !== "undefined") {
                    this.values.entity_server_settings.backupDirectoryPath = newBackupDirectoryPath;
                    this.saveSettings();
                }
            }
        },
        saveCheckInterval: {
            get (): string {
                return this.values.entity_server_settings?.persistInterval ?? "error";
            },
            set (newSaveCheckInterval: string): void {
                if (typeof this.values.entity_server_settings?.persistInterval !== "undefined") {
                    this.values.entity_server_settings.persistInterval = newSaveCheckInterval;
                    this.saveSettings();
                }
            }
        },
        isNoPersistEnabled: {
            get (): boolean {
                return this.values.entity_server_settings?.NoPersist ?? false;
            },
            set (newNoPersist: boolean): void {
                if (typeof this.values.entity_server_settings?.NoPersist !== "undefined") {
                    this.values.entity_server_settings.NoPersist = newNoPersist;
                    this.saveSettings();
                }
            }
        },
        isNoBackupEnabled: {
            get (): boolean {
                return this.values.entity_server_settings?.NoBackup ?? false;
            },
            set (newNoBackup: boolean): void {
                if (typeof this.values.entity_server_settings?.NoBackup !== "undefined") {
                    this.values.entity_server_settings.NoBackup = newNoBackup;
                    this.saveSettings();
                }
            }
        },
        statusHost: {
            get (): string {
                return this.values.entity_server_settings?.statusHost ?? "error";
            },
            set (newStatusHost: string): void {
                if (typeof this.values.entity_server_settings?.statusHost !== "undefined") {
                    this.values.entity_server_settings.statusHost = newStatusHost;
                    this.saveSettings();
                }
            }
        },
        statusPort: {
            get (): string {
                return this.values.entity_server_settings?.statusPort ?? "error";
            },
            set (newStatusPort: string): void {
                if (typeof this.values.entity_server_settings?.statusPort !== "undefined") {
                    this.values.entity_server_settings.statusPort = newStatusPort;
                    this.saveSettings();
                }
            }
        },
        isPersistFileDownloadEnabled: {
            get (): boolean {
                return this.values.entity_server_settings?.persistFileDownload ?? false;
            },
            set (newPersistFileDownload: boolean): void {
                if (typeof this.values.entity_server_settings?.persistFileDownload !== "undefined") {
                    this.values.entity_server_settings.persistFileDownload = newPersistFileDownload;
                    this.saveSettings();
                }
            }
        },
        isEditLoggingEnabled: {
            get (): boolean {
                return this.values.entity_server_settings?.wantEditLogging ?? false;
            },
            set (newEditLogging: boolean): void {
                if (typeof this.values.entity_server_settings?.wantEditLogging !== "undefined") {
                    this.values.entity_server_settings.wantEditLogging = newEditLogging;
                    this.saveSettings();
                }
            }
        },
        isTerseEditLoggingEnabled: {
            get (): boolean {
                return this.values.entity_server_settings?.wantTerseEditLogging ?? false;
            },
            set (newTerseEditLogging: boolean): void {
                if (typeof this.values.entity_server_settings?.wantTerseEditLogging !== "undefined") {
                    this.values.entity_server_settings.wantTerseEditLogging = newTerseEditLogging;
                    this.saveSettings();
                }
            }
        },
        isVerboseDebugEnabled: {
            get (): boolean {
                return this.values.entity_server_settings?.verboseDebug ?? false;
            },
            set (newVerboseDebug: boolean): void {
                if (typeof this.values.entity_server_settings?.verboseDebug !== "undefined") {
                    this.values.entity_server_settings.verboseDebug = newVerboseDebug;
                    this.saveSettings();
                }
            }
        },
        isReceivingDebugEnabled: {
            get (): boolean {
                return this.values.entity_server_settings?.debugReceiving ?? false;
            },
            set (newReceivingDebug: boolean): void {
                if (typeof this.values.entity_server_settings?.debugReceiving !== "undefined") {
                    this.values.entity_server_settings.debugReceiving = newReceivingDebug;
                    this.saveSettings();
                }
            }
        },
        isSendingDebugEnabled: {
            get (): boolean {
                return this.values.entity_server_settings?.debugSending ?? false;
            },
            set (newSendingDebug: boolean): void {
                if (typeof this.values.entity_server_settings?.debugSending !== "undefined") {
                    this.values.entity_server_settings.debugSending = newSendingDebug;
                    this.saveSettings();
                }
            }
        },
        isTimestampDebugEnabled: {
            get (): boolean {
                return this.values.entity_server_settings?.debugTimestampNow ?? false;
            },
            set (newTimestampDebug: boolean): void {
                if (typeof this.values.entity_server_settings?.debugTimestampNow !== "undefined") {
                    this.values.entity_server_settings.debugTimestampNow = newTimestampDebug;
                    this.saveSettings();
                }
            }
        },
        clockSkew: {
            get (): string {
                return this.values.entity_server_settings?.clockSkew ?? "error";
            },
            set (newclockSkew: string): void {
                if (typeof this.values.entity_server_settings?.clockSkew !== "undefined") {
                    this.values.entity_server_settings.clockSkew = newclockSkew;
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
