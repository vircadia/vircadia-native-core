<template>
    <!-- Broadcasting Settings -->
    <q-card class="my-card q-ma-sm">
        <q-card-section>
            <div class="text-h5 text-center text-weight-bold q-mb-sm">Broadcasting</div>
            <q-separator />
            <!-- ADVANCED SETTINGS SECTION -->
            <q-expansion-item v-model="isWordPressSettingsToggled" class="q-mt-md text-subtitle1" popup icon="settings" label="Advanced Settings">
                <q-card>
                    <!-- Broadcasted Users section -->
                    <q-card-section>
                        <p class="q-mb-xs text-body1 text-weight-bold">Broadcasted Users</p>
                        <div class="q-mt-xs text-caption text-grey-5">Users that are broadcasted to downstream servers.</div>
                        <q-table dark class="bg-grey-9" :rows="rows">
                            <template v-slot:header>
                                <q-tr class="bg-primary">
                                    <q-th class="text-left">User</q-th>
                                    <q-th auto-width></q-th> <!-- Empty column for delete buttons -->
                                </q-tr>
                            </template>
                            <template v-slot:body>
                                <q-tr v-for="(broadcastedUser, index) in users" :key="broadcastedUser">
                                    <q-td>{{ broadcastedUser }}</q-td>
                                    <q-td class="text-center"><q-btn size="sm" color="negative" icon="delete" class="q-px-xs" @click="onShowConfirmDeleteDialogue('users', index, broadcastedUser)" round /></q-td>
                                </q-tr>
                            </template>
                            <template v-slot:bottom-row>
                                <q-tr>
                                    <q-td>
                                        <q-input v-model="newRowNames.users" class="no-margin no-padding text-subtitle2 text-white" standout="bg-primary text-white" label="New User" dense/>
                                    </q-td>
                                    <q-td class="text-center">
                                        <q-btn @click="onAddRow('users')" color="positive"><q-icon name="add" size="sm"/></q-btn>
                                    </q-td>
                                </q-tr>
                            </template>
                        </q-table>
                    </q-card-section>
                    <!-- Receiving Servers section -->
                    <q-card-section>
                        <p class="q-mb-xs text-body1 text-weight-bold">Receiving Servers</p>
                        <div class="q-mt-xs text-caption text-grey-5">Servers that receive data for broadcasted users.</div>
                        <q-table dark class="bg-grey-9" :rows="rows">
                            <template v-slot:header>
                                <q-tr class="bg-primary">
                                    <q-th class="text-left">Address</q-th>
                                    <q-th class="text-left">Port</q-th>
                                    <q-th class="text-left">Server Type</q-th>
                                    <q-th auto-width></q-th> <!-- Empty column for delete buttons -->
                                </q-tr>
                            </template>
                            <template v-slot:body>
                                <q-tr v-for="(option, index) in downstreamServers" :key="option.address">
                                    <q-td>{{ option.address }}</q-td>
                                    <q-td>{{ option.port }}</q-td>
                                    <q-td>{{ option.server_type }}</q-td>
                                    <q-td class="text-center"><q-btn size="sm" color="negative" icon="delete" class="q-px-xs" @click="onShowConfirmDeleteDialogue('downstreamServers', index, option.address)" round /></q-td>
                                </q-tr>
                            </template>
                            <template v-slot:bottom-row>
                                <q-tr>
                                    <q-td>
                                        <q-input v-model="newRowNames.downstreamServers.address" class="no-margin no-padding text-subtitle2 text-white" standout="bg-primary text-white" label="New Address" dense/>
                                    </q-td>
                                    <q-td>
                                        <q-input v-model="newRowNames.downstreamServers.port" class="no-margin no-padding text-subtitle2 text-white" standout="bg-primary text-white" label="New Port" dense/>
                                    </q-td>
                                    <q-td>
                                        <q-select standout="bg-primary text-white" color="primary" v-model="newRowNames.downstreamServers.server_type" :options="serverTypeOptions" transition-show="jump-up" transition-hide="jump-down" dense/>
                                    </q-td>
                                    <q-td class="text-center">
                                        <q-btn @click="onAddRow('downstreamServers')" color="positive"><q-icon name="add" size="sm"/></q-btn>
                                    </q-td>
                                </q-tr>
                            </template>
                        </q-table>
                    </q-card-section>
                    <!-- Broadcasting Servers section -->
                    <q-card-section>
                        <p class="q-mb-xs text-body1 text-weight-bold">Broadcasting Servers</p>
                        <div class="q-mt-xs text-caption text-grey-5">Servers that broadcast data to this domain.</div>
                        <q-table dark class="bg-grey-9" :rows="rows">
                            <template v-slot:header>
                                <q-tr class="bg-primary">
                                    <q-th class="text-left">Address</q-th>
                                    <q-th class="text-left">Port</q-th>
                                    <q-th class="text-left">Server Type</q-th>
                                    <q-th auto-width></q-th> <!-- Empty column for delete buttons -->
                                </q-tr>
                            </template>
                            <template v-slot:body>
                                <q-tr v-for="(option, index) in upstreamServers" :key="option.address">
                                    <q-td>{{ option.address }}</q-td>
                                    <q-td>{{ option.port }}</q-td>
                                    <q-td>{{ option.server_type }}</q-td>
                                    <q-td class="text-center"><q-btn size="sm" color="negative" icon="delete" class="q-px-xs" @click="onShowConfirmDeleteDialogue('upstreamServers', index, option.address)" round /></q-td>
                                </q-tr>
                            </template>
                            <template v-slot:bottom-row>
                                <q-tr>
                                    <q-td>
                                        <q-input v-model="newRowNames.upstreamServers.address" class="no-margin no-padding text-subtitle2 text-white" standout="bg-primary text-white" label="New Address" dense/>
                                    </q-td>
                                    <q-td>
                                        <q-input v-model="newRowNames.upstreamServers.port" class="no-margin no-padding text-subtitle2 text-white" standout="bg-primary text-white" label="New Port" dense/>
                                    </q-td>
                                    <q-td>
                                        <q-select standout="bg-primary text-white" color="primary" v-model="newRowNames.upstreamServers.server_type" :options="serverTypeOptions" transition-show="jump-up" transition-hide="jump-down" dense/>
                                    </q-td>
                                    <q-td class="text-center">
                                        <q-btn @click="onAddRow('upstreamServers')" color="positive"><q-icon name="add" size="sm"/></q-btn>
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
            </q-expansion-item>
            <!-- *END* ADVANCED SETTINGS SECTION *END* -->
        </q-card-section>
    </q-card>
    <!-- *END* Broadcasting Settings *END* -->
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { Settings } from "@Modules/domain/settings";
import type { BroadcastingSaveSettings, SettingsValues, DownstreamServer, UpstreamServer } from "@Modules/domain/interfaces/settings";

type settingTypes = "users" | "downstreamServers" | "upstreamServers";

export default defineComponent({
    name: "BroadcastingSettings",
    data () {
        return {
            rows: [{}],
            isWordPressSettingsToggled: false,
            values: {} as SettingsValues,
            serverTypeOptions: ["Audio Mixer", "Avatar Mixer "],
            newRowNames: {
                users: "",
                downstreamServers: { address: "", port: "", server_type: "Audio Mixer " } as DownstreamServer,
                upstreamServers: { address: "", port: "", server_type: "Audio Mixer" } as UpstreamServer
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
            if (settingType === "users") {
                const changedSetting = [...this.users];
                changedSetting.splice(index, 1);
                this.users = changedSetting;
            } else {
                const changedSetting = [...this[settingType]];
                changedSetting.splice(index, 1);
                this[settingType] = changedSetting;
            }
        },
        onAddRow (settingType: settingTypes) {
            if (settingType === "users") {
                this.users = [...this.users, this.newRowNames.users];
                this.newRowNames[settingType] = "";
            } else {
                this[settingType] = [...this[settingType], this.newRowNames[settingType]];
                this.newRowNames[settingType] = { address: "", port: "", server_type: "Audio Mixer" };
            }
        },
        async refreshSettingsValues (): Promise<void> {
            this.values = await Settings.getValues();
        },
        saveSettings (): void {
            const settingsToCommit: BroadcastingSaveSettings = {
                "broadcasting": {
                    "users": this.users,
                    "downstream_servers": this.downstreamServers,
                    "upstream_servers": this.upstreamServers
                }
            };
            Settings.automaticCommitSettings(settingsToCommit);
        }
    },
    computed: {
        users: {
            get (): string[] {
                return this.values.broadcasting?.users ?? [];
            },
            set (newUsers: string[]): void {
                if (typeof this.values.broadcasting?.users !== "undefined") {
                    this.values.broadcasting.users = newUsers;
                    this.saveSettings();
                }
            }
        },
        downstreamServers: {
            get (): DownstreamServer[] {
                return this.values.broadcasting?.downstream_servers ?? [];
            },
            set (newDownstreamServers: DownstreamServer[]): void {
                if (typeof this.values.broadcasting?.downstream_servers !== "undefined") {
                    this.values.broadcasting.downstream_servers = newDownstreamServers;
                    this.saveSettings();
                }
            }
        },
        upstreamServers: {
            get (): UpstreamServer[] {
                return this.values.broadcasting?.upstream_servers ?? [];
            },
            set (newUpstreamServers: UpstreamServer[]): void {
                if (typeof this.values.broadcasting?.upstream_servers !== "undefined") {
                    this.values.broadcasting.upstream_servers = newUpstreamServers;
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
