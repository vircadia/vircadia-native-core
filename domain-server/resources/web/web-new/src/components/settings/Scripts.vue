<template>
    <div>
        <!-- Scripts Settings -->
        <q-card class="my-card q-mt-md">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold q-mb-sm">Scripts</div>
                <q-separator />
                <q-card>
                    <!-- Persistent Scripts Table section -->
                    <q-card-section>
                        <p class="q-mb-xs text-body1 text-weight-bold">Persistent Scripts</p>
                        <div class="q-mt-xs text-caption text-grey-5">Add the URLs for scripts that you would like to ensure are always running in your domain.</div>
                        <q-table dark class="bg-grey-9" :rows="rows">
                            <template v-slot:header>
                                <q-tr class="bg-primary">
                                    <q-th class="text-left">Script URL</q-th>
                                    <q-th class="text-left"># instances</q-th>
                                    <q-th class="text-left">Pool</q-th>
                                    <q-th auto-width></q-th> <!-- Empty column for delete buttons -->
                                </q-tr>
                            </template>
                            <template v-slot:body>
                                <q-tr v-for="(script, index) in persistentScripts" :key="script.url">
                                    <q-td>{{ script.url }}</q-td>
                                    <q-td>{{ script.num_instances }}</q-td>
                                    <q-td>{{ script.pool }}</q-td>
                                    <q-td class="text-center"><q-btn @click="onShowConfirmDeleteDialogue(script.url, index)" size="sm" color="negative" icon="delete" class="q-px-xs" round /></q-td>
                                </q-tr>
                            </template>
                            <template v-slot:bottom-row>
                                <q-tr>
                                    <q-td>
                                        <q-input v-model="newScript.url" class="no-margin no-padding text-subtitle2 text-white" standout="bg-primary text-white" label="New Script URL" dense/>
                                    </q-td>
                                    <q-td>
                                        <q-input v-model="newScript.num_instances" class="no-margin no-padding text-subtitle2 text-white" standout="bg-primary text-white" label="Instances" dense/>
                                    </q-td>
                                    <q-td>
                                        <q-input v-model="newScript.pool" class="no-margin no-padding text-subtitle2 text-white" standout="bg-primary text-white" label="Pool" dense/>
                                    </q-td>
                                    <q-td class="text-center">
                                        <q-btn @click="onAddScript" color="positive"><q-icon name="add" size="sm"/></q-btn>
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
                            <p class="text-h6 text-bold text-white full-width"><q-avatar icon="mdi-alert" class="q-mr-sm" text-color="warning" size="20px" font-size="20px"/>Delete script <span class="text-warning">{{confirmDeleteDialogue.scriptToDelete}}</span>?</p>
                            <p class="text-body2">WARNING: This cannot be undone.</p>
                        </q-card-section>
                        <q-card-actions align="center">
                            <q-btn flat label="Cancel" @click="onHideConfirmDeleteDialogue()"/>
                            <q-btn flat label="Delete" @click="onDeleteScript(confirmDeleteDialogue.index)"/>
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
import { ContentSettingsValues, PersistentScript, ScriptsSaveSetting } from "@/src/modules/domain/interfaces/contentSettings";
import { ContentSettings } from "@Modules/domain/contentSettings";

export default defineComponent({
    name: "Scripts",

    data () {
        return {
            rows: [{}],
            values: {} as ContentSettingsValues,
            newScript: { url: "", num_instances: "1", pool: "" } as PersistentScript,
            confirmDeleteDialogue: { show: false, scriptToDelete: "", index: -1 }
        };
    },
    methods: {
        onShowConfirmDeleteDialogue (scriptToDelete: string, indexToDelete: number): void {
            this.confirmDeleteDialogue = { show: true, scriptToDelete: scriptToDelete, index: indexToDelete };
        },
        onHideConfirmDeleteDialogue (): void {
            this.confirmDeleteDialogue = { show: false, scriptToDelete: "", index: -1 };
        },
        onDeleteScript (indexToDelete: number): void {
            this.onHideConfirmDeleteDialogue();
            const changedPersistentScripts = [...this.persistentScripts];
            changedPersistentScripts.splice(indexToDelete, 1);
            this.persistentScripts = changedPersistentScripts;
        },
        onAddScript () {
            this.persistentScripts = [...this.persistentScripts, this.newScript];
            this.newScript = { url: "", num_instances: "1", pool: "" };
        },
        async refreshSettingsValues (): Promise<void> {
            this.values = await ContentSettings.getValues();
        },
        saveSettings (): void {
            const settingsToCommit: ScriptsSaveSetting = {
                "scripts": {
                    "persistent_scripts": this.persistentScripts
                }
            };
            ContentSettings.automaticCommitSettings(settingsToCommit);
        }
    },
    computed: {
        persistentScripts: {
            get (): PersistentScript[] {
                return this.values.scripts?.persistent_scripts ?? [];
            },
            set (value: PersistentScript[]): void {
                if (typeof this.values.scripts?.persistent_scripts !== "undefined") {
                    this.values.scripts.persistent_scripts = value;
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
