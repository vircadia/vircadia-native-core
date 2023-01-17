<template>
    <div>
        <!-- Automatic Content Archives  Settings -->
        <q-card class="my-card">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold q-mb-sm">Automatic Content Archives</div>
                <q-separator />
                    <q-card>
                        <!-- Rolling Backup Rules section -->
                        <q-card-section>
                            <p class="q-mb-xs text-body1 text-weight-bold">Rolling Backup Rules</p>
                            <div class="q-mt-xs text-caption text-grey-5">Define how frequently to create automatic content archives.</div>
                            <q-table dark class="bg-grey-9" :rows="rows">
                                <template v-slot:header>
                                    <q-tr class="bg-primary">
                                        <q-th class="text-left">Name</q-th>
                                        <q-th class="text-left">Backup Interval in Seconds</q-th>
                                        <q-th class="text-left">Max Rolled Backup Versions</q-th>
                                        <q-th auto-width></q-th> <!-- Empty column for delete buttons -->
                                    </q-tr>
                                </template>
                                <template v-slot:body>
                                    <q-tr v-for="(option, index) in backupRules" :key="option.Name">
                                        <q-td>{{ option.Name }}</q-td>
                                        <q-td>{{ option.backupInterval }}</q-td>
                                        <q-td>{{ option.maxBackupVersions }}</q-td>
                                        <q-td class="text-center"><q-btn size="sm" color="negative" icon="delete" class="q-px-xs" @click="onShowConfirmDeleteDialogue('backupRules', index, option.Name)" round /></q-td>
                                    </q-tr>
                                </template>
                                <template v-slot:bottom-row>
                                    <q-tr>
                                        <q-td>
                                            <q-input v-model="newRowNames.backupRules.Name" class="no-margin no-padding text-subtitle2 text-white" standout="bg-primary text-white" label="New Backup Rule Name" dense/>
                                        </q-td>
                                        <q-td>
                                            <q-input v-model="newRowNames.backupRules.backupInterval" class="no-margin no-padding text-subtitle2 text-white" standout="bg-primary text-white" label="Backup Interval (seconds)" dense/>
                                        </q-td>
                                        <q-td>
                                            <q-input v-model="newRowNames.backupRules.maxBackupVersions" class="no-margin no-padding text-subtitle2 text-white" standout="bg-primary text-white" label="Maximum Rolled Backup Versions" dense/>
                                        </q-td>
                                        <q-td class="text-center">
                                            <q-btn @click="onAddRow('backupRules')" color="positive"><q-icon name="add" size="sm"/></q-btn>
                                        </q-td>
                                    </q-tr>
                                </template>
                            </q-table>
                        </q-card-section>
                    </q-card>
                    <!-- CONFIRM DELETE PERMISSION DIALOGUE -->
                    <q-dialog v-model="confirmDeleteDialogue.show" persistent>
                        <q-card class="bg-primary q-pa-md">
                            <q-card-section class="row items-center">
                                <p class="text-h6 text-bold text-white full-width"><q-avatar icon="mdi-alert" class="q-mr-sm" text-color="warning" size="20px" font-size="20px"/>Delete <span class="text-warning">{{confirmDeleteDialogue.thingToDelete}}</span>?</p>
                                <p class="text-body2">WARNING: This cannot be undone.</p>
                            </q-card-section>
                            <q-card-actions align="center">
                                <q-btn flat label="Cancel" @click="onHideConfirmDeleteDialogue()"/>
                                <q-btn flat label="Delete" @click="onDeleteRow(confirmDeleteDialogue.permissionType, confirmDeleteDialogue.index)"/>
                            </q-card-actions>
                        </q-card>
                    </q-dialog>
            </q-card-section>
        </q-card>
        <!-- *END* Description Settings *END* -->
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { Settings } from "@Modules/domain/settings";
import { AutomaticContentArchivesSaveSettings, BackupRule, SettingsValues } from "@/src/modules/domain/interfaces/settings";

type settingTypes = "backupRules";

export default defineComponent({
    name: "AutomaticContentArchivesSettings",

    data () {
        return {
            rows: [{}],
            isWordPressSettingsToggled: false,
            values: {} as SettingsValues,
            newRowNames: {
                backupRules: { Name: "", backupInterval: 0, maxBackupVersions: 0 } as BackupRule
            },
            confirmDeleteDialogue: { show: false, thingToDelete: "", index: -1, permissionType: "" as settingTypes }
        };
    },
    methods: {
        onShowConfirmDeleteDialogue (permissionType: settingTypes, index: number, thingToDelete: string): void {
            this.confirmDeleteDialogue = { show: true, thingToDelete: thingToDelete, index: index, permissionType: permissionType };
        },
        onHideConfirmDeleteDialogue (): void {
            this.confirmDeleteDialogue = { show: false, thingToDelete: "", index: -1, permissionType: "" as settingTypes };
        },
        onDeleteRow (settingType: settingTypes, index: number): void {
            this.onHideConfirmDeleteDialogue();
            const changedSetting = [...this[settingType]];
            changedSetting.splice(index, 1);
            this[settingType] = changedSetting;
        },
        onAddRow (settingType: settingTypes) {
            this[settingType] = [...this[settingType], this.newRowNames[settingType]];
            this.newRowNames[settingType] = { Name: "", backupInterval: 0, maxBackupVersions: 0 };
        },
        async refreshSettingsValues (): Promise<void> {
            this.values = await Settings.getValues();
        },
        saveSettings (): void {
            const settingsToCommit: AutomaticContentArchivesSaveSettings = {
                "automatic_content_archives": {
                    "backup_rules": this.backupRules
                }
            };
            Settings.automaticCommitSettings(settingsToCommit);
        }
    },
    computed: {
        backupRules: {
            get (): BackupRule[] {
                return this.values.automatic_content_archives?.backup_rules ?? [];
            },
            set (newBackupRules: BackupRule[]): void {
                if (typeof this.values.automatic_content_archives?.backup_rules !== "undefined") {
                    this.values.automatic_content_archives.backup_rules = newBackupRules;
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
