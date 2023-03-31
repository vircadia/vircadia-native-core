<template>
    <!-- Audio Environment Settings -->
    <q-card class="my-card q-mt-md">
        <q-card-section>
            <div class="text-h5 text-center text-weight-bold q-mb-sm">Audio Environment</div>
            <q-separator />
            <q-card>
                <!-- Default Domain Attenuation section -->
                <q-card-section>
                    <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="defaultDomainAttenuation" label="Default Domain Attenuation"/>
                    <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Factor between 0 and 1.0 (0: No attenuation, 1.0: extreme attenuation)</div>
                </q-card-section>
                <!-- Zones Table Section -->
                <q-card-section>
                    <p class="q-mb-xs text-body1 text-weight-bold">Zones</p>
                    <div class="q-mt-xs text-caption text-grey-5">In this table you can define a set of zones in which you can specify various audio properties.</div>
                    <q-table dark class="bg-grey-9" :rows="rows">
                        <template v-slot:header>
                            <q-tr class="bg-primary">
                                <q-th class="text-left">Name</q-th>
                                <q-th class="text-left">X start</q-th>
                                <q-th class="text-left">X end</q-th>
                                <q-th class="text-left">Y start</q-th>
                                <q-th class="text-left">Y end</q-th>
                                <q-th class="text-left">Z start</q-th>
                                <q-th class="text-left">Z end</q-th>
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
    <!-- *END* Audio Environment Settings *END* -->
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { ContentSettings } from "@Modules/domain/contentSettings";
import type { ContentSettingsValues, Path, PathsSaveSetting } from "@Modules/domain/interfaces/contentSettings";

export default defineComponent({
    name: "AudioEnvironment",
    data () {
        return {
            rows: [{}],
            values: {} as ContentSettingsValues,
            serverTypeOptions: ["Audio Mixer", "Avatar Mixer "],
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
        async refreshSettingsValues (): Promise<void> {
            this.values = await ContentSettings.getValues();
        },
        saveSettings (): void {
            const settingsToCommit: PathsSaveSetting = {
                paths: this.paths
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
        },
        defaultDomainAttenuation: {
            get (): string {
                return this.values?.audio_env?.attenuation_per_doubling_in_distance ?? "";
            },
            set (newAttenuation: string): void {
                if (typeof this.values?.audio_env?.attenuation_per_doubling_in_distance !== "undefined") {
                    this.values.audio_env.attenuation_per_doubling_in_distance = newAttenuation;
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
