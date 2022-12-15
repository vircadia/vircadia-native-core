<template>
    <div>
        <!-- Security Settings -->
        <q-card class="my-card q-mt-md">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold q-mb-sm">Security</div>
                <q-separator />
                <!-- ADVANCED SETTINGS SECTION -->
                <q-expansion-item v-model="isSecuritySettingsToggled" class="q-mt-md text-subtitle1" popup icon="settings" label="Advanced Settings">
                    <q-card>
                        <!-- HTTP Username section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="HTTPUsername" label="HTTP Username"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Username used for basic HTTP authentication.</div>
                        </q-card-section>
                        <!-- HTTP Password section -->
                        <!-- <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="HTTPPassword" label="HTTP Password"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Password used for basic HTTP authentication. Leave this alone if you do not want to change it.</div>
                        </q-card-section> -->
                        <!-- Approved Script and QML URLs section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="approvedURLs" label="Approved Script and QML URLs (Not Enabled)"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">These URLs will be sent to the Interface as safe URLs to allow through the whitelist if the Interface has this security option enabled.</div>
                        </q-card-section>
                        <!-- Maximum User Capacity section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model.number="maximumCapacity" label="Maximum User Capacity" type="number" :rules="[val => (val >= 0) || 'Maximum capacity cannot be negative']"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">The limit on how many users can be connected at once (0 means no limit). Avatars connected from the same machine will not count towards this limit.</div>
                        </q-card-section>
                        <!-- Redirect to Location on Maximum Capacity section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="maximumCapacityRedirect" label="Redirect to Location on Maximum Capacity"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">The location to redirect users to when the maximum number of avatars are connected.</div>
                        </q-card-section>
                        <!-- Redirect to Location on Maximum Capacity section -->
                        <q-card-section>
                            <h5 class="q-mx-none q-my-sm text-weight-bold">Domain-Wide User Permissions</h5>
                            <p class="text-body2">Indicate which types of users can have which <span class="text-accent cursor-pointer">domain-wide permissions<q-tooltip>yes</q-tooltip></span></p>
                            <q-markup-table dark class="bg-grey-9">
                                <thead>
                                    <tr class="bg-primary">
                                        <th class="text-center">Type of User</th>
                                        <th class="text-center">Connect</th>
                                        <th class="text-center">Avatar Entities</th>
                                        <th class="text-center">Lock / Unlock</th>
                                        <th class="text-center">Rez</th>
                                        <th class="text-center">Rez Temporary</th>
                                        <th class="text-center">Rez Certified</th>
                                        <th class="text-center">Rez Temporary Certified</th>
                                        <th class="text-center">Write Assets</th>
                                        <th class="text-center">Ignore Max Capacity</th>
                                        <th class="text-center">Kick Users</th>
                                        <th class="text-center">Replace Content</th>
                                        <th class="text-center">Get and Set Private User Data</th>
                                    </tr>
                                </thead>
                                <tbody>
                                    <tr v-for="(userType) in standardPermissions" :key="userType.permissions_id">
                                        <td class="text-center">{{userType.permissions_id}}</td>
                                        <td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_connect" @change="permissionChange"/></td>
                                        <td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_rez_avatar_entities"/></td>
                                        <td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_adjust_locks"/></td>
                                        <td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_rez"/></td>
                                        <td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_rez_tmp"/></td>
                                        <td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_rez_certified"/></td>
                                        <td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_rez_tmp_certified"/></td>
                                        <td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_write_to_asset_server"/></td>
                                        <td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_connect_past_max_capacity"/></td>
                                        <td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_kick"/></td>
                                        <td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_replace_content"/></td>
                                        <td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_get_and_set_private_user_data"/></td>
                                    </tr>
                                </tbody>
                            </q-markup-table>
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
import { SettingsValues, SecuritySaveSettings, StandardPermission } from "@/src/modules/domain/interfaces/settings";

export default defineComponent({
    name: "SecuritySettings",
    data () {
        return {
            isSecuritySettingsToggled: false,
            values: {} as SettingsValues,
            standardPermissions: [] as StandardPermission[]
        };
    },
    methods: {
        // onStandardPermissionChange<Permission extends keyof StandardPermission> (index: number, permission: Permission, value: StandardPermission[Permission]): void {
        //     const changedPermission = this.standardPermissions[index];
        //     // standardPermissions.splice(index, 1, )
        //     if (typeof changedPermission[permission] === "boolean") {
        //         changedPermission[permission] = !changedPermission[permission];
        //     }
        //     changedPermission[permission] = !changedPermission[permission];
        //     this.standardPermissions.splice(index, 1, this.standardPermissions[index]);
        // },
        permissionChange (value: boolean): void {
            console.log(value);
        },
        async refreshSettingsValues (): Promise<void> {
            this.values = await Settings.getValues();
            this.standardPermissions = this.values.security?.standard_permissions ?? [];
        },
        saveSettings (): void {
            const settingsToCommit: SecuritySaveSettings = {
                "security": {
                    "http_username": this.HTTPUsername,
                    "approved_safe_urls": this.approvedURLs
                }
            };
            Settings.automaticCommitSettings(settingsToCommit);
        }
    },
    watch: {
        standardPermission: {
            handler (newValue, oldValue) {
                console.log(newValue, oldValue);
            },
            deep: true
        }
    },
    computed: {
        HTTPUsername: {
            get (): string {
                return this.values.security?.http_username ?? "error";
            },
            set (newUsername: string): void {
                if (typeof this.values.security?.http_username !== "undefined") {
                    this.values.security.http_username = newUsername;
                    this.saveSettings();
                }
            }
        },
        // HTTPPassword: {
        //     get (): string {
        //         return this.values.security?.http_password ?? "error";
        //     },
        //     set (newPassword: string): void {
        //         if (typeof this.values.security?.http_password !== "undefined") {
        //             this.values.security.http_password = newPassword;
        //             this.saveSettings();
        //         }
        //     }
        // }
        approvedURLs: {
            get (): string {
                return this.values.security?.approved_safe_urls ?? "error";
            },
            set (newApprovedURLs: string): void {
                if (typeof this.values.security?.approved_safe_urls !== "undefined") {
                    this.values.security.approved_safe_urls = newApprovedURLs;
                    this.saveSettings();
                }
            }
        },
        maximumCapacity: {
            get (): number {
                // checks is not type undefined and converts from string to int (v-model.number already typecasts to number, this may be unnessesary)
                return typeof this.values.security?.maximum_user_capacity !== "undefined" ? parseInt(this.values.security.maximum_user_capacity) : 0;
            },
            set (newMaximumCapacity: number): void {
                if (typeof this.values.security?.maximum_user_capacity !== "undefined") {
                    this.values.security.maximum_user_capacity = `${newMaximumCapacity}`;
                    this.saveSettings();
                }
            }
        },
        maximumCapacityRedirect: {
            get (): string {
                return this.values.security?.maximum_user_capacity_redirect_location ?? "error";
            },
            set (newMaximumCapacityRedirect: string): void {
                if (typeof this.values.security?.maximum_user_capacity_redirect_location !== "undefined") {
                    this.values.security.maximum_user_capacity_redirect_location = newMaximumCapacityRedirect;
                    this.saveSettings();
                }
            }
        }
        // standardPermissions: {
        //     get (): StandardPermission[] {
        //         return this.values.security?.standard_permissions ?? [];
        //     },
        //     set (newStandardPermissions: StandardPermission[]): void {
        //         console.log("changed");
        //         if (typeof this.values.security?.standard_permissions !== "undefined") {
        //             this.values.security.standard_permissions = newStandardPermissions;
        //             this.saveSettings();
        //         }
        //     }
        // }
    },
    beforeMount () {
        this.refreshSettingsValues();
    }
});
</script>

<style scoped>
table {
    table-layout: fixed;
}
</style>
