<template>
    <div>
        <!-- Paths Settings -->
        <q-card class="my-card q-mt-md">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold q-mb-sm">Paths</div>
                <q-separator />
                <q-card>
                    <!-- Paths Table section -->
                    <q-card-section>
                        <p class="q-mb-xs text-body1 text-weight-bold">Paths</p>
                        <div class="q-mt-xs text-caption text-grey-5">Clients can enter a path to reach an exact viewpoint in your domain. Add rows to the table below to map a path to a viewpoint.<br/>The index path ( / ) is where clients will enter if they do not enter an explicit path.</div>
                        <q-table dark class="bg-grey-9" :rows="rows">
                            <template v-slot:header>
                                <q-tr class="bg-primary">
                                    <q-th class="text-left">Path</q-th>
                                    <q-th class="text-left">Viewpoint</q-th>
                                    <q-th auto-width></q-th> <!-- Empty column for delete buttons -->
                                </q-tr>
                            </template>
                            <template v-slot:body>
                                <q-tr v-for="path of Object.entries(paths)" :key="path[0]">
                                    <q-td>{{ path[0] }}</q-td>
                                    <q-td>{{ path[1].viewpoint }}</q-td>
                                    <q-td class="text-center"><q-btn @click="onShowConfirmDeleteDialogue(path[0])" size="sm" color="negative" icon="delete" class="q-px-xs" round /></q-td>
                                </q-tr>
                            </template>
                            <template v-slot:bottom-row>
                                <q-tr>
                                    <q-td>
                                        <q-input v-model="newPath.path" class="no-margin no-padding text-subtitle2 text-white" standout="bg-primary text-white" label="New Path" hint="/" dense/>
                                    </q-td>
                                    <q-td>
                                        <q-input v-model="newPath.viewpoint" class="no-margin no-padding text-subtitle2 text-white" standout="bg-primary text-white" label="Viewpoint" hint="/0,0,0" dense/>
                                    </q-td>
                                    <q-td class="text-center">
                                        <q-btn color="positive"><q-icon name="add" size="sm"/></q-btn>
                                    </q-td>
                                </q-tr>
                            </template>
                        </q-table>
                    </q-card-section>
                </q-card>
                <!-- CONFIRM DELETE PATH DIALOGUE -->
                <q-dialog v-model="confirmDeleteDialogue.show" persistent>
                    <q-card class="bg-primary q-pa-md">
                        <q-card-section class="row items-center">
                            <p class="text-h6 text-bold text-white full-width"><q-avatar icon="mdi-alert" class="q-mr-sm" text-color="warning" size="20px" font-size="20px"/>Delete <span class="text-warning">{{confirmDeleteDialogue.pathName}}</span>?</p>
                            <p class="text-body2">WARNING: This cannot be undone.</p>
                        </q-card-section>
                        <q-card-actions align="center">
                            <q-btn flat label="Cancel" @click="onHideConfirmDeleteDialogue()"/>
                            <q-btn flat label="Delete" @click="onDeletePath(confirmDeleteDialogue.pathName)"/>
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
import { ContentSettings } from "@Modules/domain/contentSettings";
import type { ContentSettingsValues, Path, PathsSaveSetting } from "@Modules/domain/interfaces/contentSettings";

export default defineComponent({
    name: "Paths",
    data () {
        return {
            rows: [{}],
            values: {} as ContentSettingsValues,
            newPath: { path: "", viewpoint: "" },
            confirmDeleteDialogue: { show: false, pathName: "" }
        };
    },
    methods: {
        onShowConfirmDeleteDialogue (pathName: string): void {
            this.confirmDeleteDialogue = { show: true, pathName: pathName };
        },
        onHideConfirmDeleteDialogue (): void {
            this.confirmDeleteDialogue = { show: false, pathName: "" };
        },
        onDeletePath (pathToDelete: string): void {
            this.onHideConfirmDeleteDialogue();
            const changedPaths = { ...this.paths };
            delete changedPaths[pathToDelete];
            this.paths = changedPaths;
        },
        // onDeleteRow (settingType: settingTypes, index: number): void {
        //     this.onHideConfirmDeleteDialogue();
        //     if (settingType === "users") {
        //         const changedSetting = [...this.users];
        //         changedSetting.splice(index, 1);
        //         this.users = changedSetting;
        //     } else {
        //         const changedSetting = [...this[settingType]];
        //         changedSetting.splice(index, 1);
        //         this[settingType] = changedSetting;
        //     }
        // },
        // onAddRow (settingType: settingTypes): void {
        //     this[settingType] = [...this[settingType], this.newRowNames[settingType]];
        //     this.newRowNames[settingType] = { address: "", port: "", server_type: "Audio Mixer" };
        // },
        async refreshSettingsValues (): Promise<void> {
            this.values = await ContentSettings.getValues();
        },
        saveSettings (): void {
            const output = {} as { [key: string]: Path };
            for (const entry of Object.entries(this.paths)) {
                output[entry[0].replace("/", "")] = entry[1];
            }

            const settingsToCommit: PathsSaveSetting = {
                "paths": output
            };
            ContentSettings.automaticCommitSettings(settingsToCommit);
        }
    },
    computed: {
        paths: {
            get (): Record<string, Path> {
                return this.values?.paths ?? {};
            },
            set (newPaths: Record<string, Path>): void {
                if (typeof this.values?.paths !== "undefined") {
                    this.values.paths = newPaths;
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
