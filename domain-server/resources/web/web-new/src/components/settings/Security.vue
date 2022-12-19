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
                        <!-- Domain-Wide User Permissions section -->
                        <q-card-section>
                            <h5 class="q-mx-none q-my-sm text-weight-bold">Domain-Wide User Permissions</h5>
                            <p class="text-body2">Indicate which types of users can have which <a href="https://docs.vircadia.com/host/configure-settings/permission-settings.html#user-permissions" target="_blank" class="text-accent">domain-wide permissions</a></p>
                            <p class="q-mb-xs q-mt-lg text-body1 text-weight-bold">Standard Permissions</p>
                            <q-table dark class="bg-grey-9" :rows="rows" hide-bottom>
                                <template v-slot:header>
                                    <q-tr class="bg-primary">
                                        <q-th class="text-center"><a class="text-white" href="https://docs.vircadia.com/host/configure-settings/permission-settings.html#standard-user-groups" target="_blank" >Type of User</a></q-th>
                                        <q-th class="text-center">Connect<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can connect to the domain</q-tooltip></q-icon></q-th>
                                        <q-th class="text-center">Avatar Entities<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can use avatar entities on the domain</q-tooltip></q-icon></q-th>
                                        <q-th class="text-center">Lock / Unlock<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can use change the "locked" property of an entity (from off➔on / on➔off)</q-tooltip></q-icon></q-th>
                                        <q-th class="text-center">Rez<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can use create new entities</q-tooltip></q-icon></q-th>
                                        <q-th class="text-center">Rez Temporary<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can create new temporary entities (with finite lifetimes)</q-tooltip></q-icon></q-th>
                                        <q-th class="text-center">Rez Certified<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can create new certified entities</q-tooltip></q-icon></q-th>
                                        <q-th class="text-center">Rez Temporary Certified<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can create temporary (finite lifetime), certified entities</q-tooltip></q-icon></q-th>
                                        <q-th class="text-center">Write Assets<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can make changes to the domain's asset-server assets</q-tooltip></q-icon></q-th>
                                        <q-th class="text-center">Ignore Max Capacity<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can connect even when the domain's maximum user capacity is reached</q-tooltip></q-icon></q-th>
                                        <q-th class="text-center">Kick Users<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can kick other users</q-tooltip></q-icon></q-th>
                                        <q-th class="text-center">Replace Content<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can replace entire content sets by wiping existing domain content</q-tooltip></q-icon></q-th>
                                        <q-th class="text-center">Get and Set Private User Data<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can get and set the privateUserData entity property</q-tooltip></q-icon></q-th>
                                    </q-tr>
                                </template>
                                <template v-slot:body>
                                    <q-tr v-for="(userType, index) in standardPermissions" :key="userType.permissions_id">
                                        <q-td class="text-center">{{userType.permissions_id}}</q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" :modelValue="userType.id_can_connect" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_connect', newValue)"/></q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_rez_avatar_entities" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_rez_avatar_entities', newValue)"/></q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_adjust_locks" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_adjust_locks', newValue)"/></q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_rez" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_rez', newValue)"/></q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_rez_tmp" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_rez_tmp', newValue)"/></q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_rez_certified" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_rez_certified', newValue)"/></q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_rez_tmp_certified" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_rez_tmp_certified', newValue)"/></q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_write_to_asset_server" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_write_to_asset_server', newValue)"/></q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_connect_past_max_capacity" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_connect_past_max_capacity', newValue)"/></q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_kick" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_kick', newValue)"/></q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_replace_content" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_replace_content', newValue)"/></q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_get_and_set_private_user_data" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_get_and_set_private_user_data', newValue)"/></q-td>
                                    </q-tr>
                                </template>
                            </q-table>
                        </q-card-section>
                        <!-- Permissions for Users in Groups section -->
                        <q-card-section>
                            <p class="q-mb-xs text-body1 text-weight-bold">Permissions for Users in Groups</p>
                            <q-table dark class="bg-grey-9" :rows="rows">
                                <template v-slot:header>
                                    <q-tr class="bg-primary">
                                        <q-th class="text-center">User Group</q-th>
                                        <q-th class="text-center">Connect<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can connect to the domain</q-tooltip></q-icon></q-th>
                                        <q-th class="text-center">Avatar Entities<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can use avatar entities on the domain</q-tooltip></q-icon></q-th>
                                        <q-th class="text-center">Lock / Unlock<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can use change the "locked" property of an entity (from off➔on / on➔off)</q-tooltip></q-icon></q-th>
                                        <q-th class="text-center">Rez<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can use create new entities</q-tooltip></q-icon></q-th>
                                        <q-th class="text-center">Rez Temporary<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can create new temporary entities (with finite lifetimes)</q-tooltip></q-icon></q-th>
                                        <q-th class="text-center">Rez Certified<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can create new certified entities</q-tooltip></q-icon></q-th>
                                        <q-th class="text-center">Rez Temporary Certified<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can create temporary (finite lifetime), certified entities</q-tooltip></q-icon></q-th>
                                        <q-th class="text-center">Write Assets<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can make changes to the domain's asset-server assets</q-tooltip></q-icon></q-th>
                                        <q-th class="text-center">Ignore Max Capacity<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can connect even when the domain's maximum user capacity is reached</q-tooltip></q-icon></q-th>
                                        <q-th class="text-center">Kick Users<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can kick other users</q-tooltip></q-icon></q-th>
                                        <q-th class="text-center">Replace Content<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can replace entire content sets by wiping existing domain content</q-tooltip></q-icon></q-th>
                                        <q-th class="text-center">Get and Set Private User Data<q-icon name="help" size="xs" class="q-ml-sm"><q-tooltip class="text-caption text-dark bg-grey-2">Whether a user can get and set the privateUserData entity property</q-tooltip></q-icon></q-th>
                                    </q-tr>
                                </template>
                                <template v-slot:body>
                                    <q-tr v-for="(userType, index) in groupPermissions" :key="userType.permissions_id">
                                        <q-td class="text-center">{{userType.permissions_id}}</q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" :modelValue="userType.id_can_connect" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_connect', newValue)"/></q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_rez_avatar_entities" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_rez_avatar_entities', newValue)"/></q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_adjust_locks" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_adjust_locks', newValue)"/></q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_rez" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_rez', newValue)"/></q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_rez_tmp" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_rez_tmp', newValue)"/></q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_rez_certified" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_rez_certified', newValue)"/></q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_rez_tmp_certified" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_rez_tmp_certified', newValue)"/></q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_write_to_asset_server" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_write_to_asset_server', newValue)"/></q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_connect_past_max_capacity" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_connect_past_max_capacity', newValue)"/></q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_kick" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_kick', newValue)"/></q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_replace_content" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_replace_content', newValue)"/></q-td>
                                        <q-td class="text-center"><q-checkbox size="sm" v-model="userType.id_can_get_and_set_private_user_data" @update:modelValue="newValue => onStandardPermissionChange(index, 'id_can_get_and_set_private_user_data', newValue)"/></q-td>
                                    </q-tr>
                                </template>
                                <template v-slot:bottom>
                                    <q-tr>
                                        <q-td colspan="100%">
                                            Bottom row
                                        </q-td>
                                    </q-tr>
                                </template>
                            </q-table>
                            <p class="q-mt-xs q-mb-none text-caption text-grey-5">For groups that are provided from WordPress you need to denote them by putting an "@" symbol in front of each item, e.g., "@silver".</p>
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
import { SettingsValues, SecuritySaveSettings, StandardPermission, GroupPermission } from "@/src/modules/domain/interfaces/settings";

export default defineComponent({
    name: "SecuritySettings",
    data () {
        return {
            rows: [{}],
            isSecuritySettingsToggled: false,
            values: {} as SettingsValues,
            newUsersInGroupName: ""
        };
    },
    methods: {
        onStandardPermissionChange (index: number, permission: keyof StandardPermission, newValue: boolean): void {
            const changedPermission = [...this.standardPermissions];
            if (permission !== "permissions_id") {
                changedPermission[index][permission] = newValue;
                this.standardPermissions = [...changedPermission];
            }
        },
        async refreshSettingsValues (): Promise<void> {
            this.values = await Settings.getValues();
        },
        saveSettings (): void {
            const settingsToCommit: SecuritySaveSettings = {
                "security": {
                    "http_username": this.HTTPUsername,
                    "approved_safe_urls": this.approvedURLs,
                    "standard_permissions": this.standardPermissions
                }
            };
            Settings.automaticCommitSettings(settingsToCommit);
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
        },
        standardPermissions: {
            get (): StandardPermission[] {
                return this.values.security?.standard_permissions ?? [];
            },
            set (newStandardPermissions: StandardPermission[]): void {
                if (typeof this.values.security?.standard_permissions !== "undefined") {
                    this.values.security.standard_permissions = newStandardPermissions;
                    this.saveSettings();
                }
            }
        },
        groupPermissions: {
            get (): GroupPermission[] {
                return this.values.security?.group_permissions ?? [];
            },
            set (newGroupPermissions: GroupPermission[]): void {
                if (typeof this.values.security?.group_permissions !== "undefined") {
                    this.values.security.group_permissions = newGroupPermissions;
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

<style scoped>
table {
    table-layout: fixed;
}
</style>
