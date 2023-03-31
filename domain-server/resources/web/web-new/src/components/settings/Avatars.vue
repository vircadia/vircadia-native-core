<template>
    <div>
        <!-- Avatars Settings -->
        <q-card class="my-card q-ma-sm">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold q-mb-sm">Avatars</div>
                <q-separator />
                <!-- ADVANCED SETTINGS SECTION -->
                <q-expansion-item v-model="isWordPressSettingsToggled" class="q-mt-md text-subtitle1" popup icon="settings" label="Advanced Settings">
                    <q-card>
                        <!-- Minimum Avatar Height (meters) section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="minAvatarHeight" label="Minimum Avatar Height (meters)"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Limits the height of avatars in your domain. Must be at least 0.009.</div>
                        </q-card-section>
                        <!-- Maximum Avatar Height (meters) section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="maxAvatarHeight" label="Maximum Avatar Height (meters)"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Limits the height of avatars in your domain. Cannot be greater than 1755.</div>
                        </q-card-section>
                        <!-- Maximum Avatar Height (meters) section
                        <q-card-section>
                            <q-range
                                class="q-mt-xl"
                                v-model="avatarHeightRange"
                                color="deep-orange"
                                label-always
                                markers
                                :min="0.009"
                                :max="1755"
                                >
                            </q-range>
                        </q-card-section> -->
                        <!-- Avatars Allowed From: section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="avatarWhitelist" label="Avatars Allowed From:"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Comma separated list of URLs (with optional paths) that avatar .fst files are allowed from. If someone attempts to use an avatar with a different domain, it will be rejected and the replacement avatar will be used. If left blank, any domain is allowed.</div>
                        </q-card-section>
                        <!-- Avatars Allowed From: section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="replacementAvatar" label="Replacement Avatar For Disallowed Avatars"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">A URL for an avatar .fst to be used when someone tries to use an avatar that is not allowed. If left blank, the generic default avatar is used.</div>
                        </q-card-section>
                    </q-card>
                </q-expansion-item>
                <!-- *END* ADVANCED SETTINGS SECTION *END* -->
            </q-card-section>
        </q-card>
        <!-- *END* WordPress OAuth2 Settings *END* -->
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { Settings } from "@Modules/domain/settings";
import type { AvatarsSaveSettings, SettingsValues } from "@Modules/domain/interfaces/settings";

export default defineComponent({
    name: "AvatarsSettings",

    data () {
        return {
            isWordPressSettingsToggled: false,
            values: {} as SettingsValues,
            avatarHeightRange: {
                min: 0,
                max: 0
            }
        };
    },
    methods: {
        async refreshSettingsValues (): Promise<void> {
            this.values = await Settings.getValues();
        },
        saveSettings (): void {
            const settingsToCommit: AvatarsSaveSettings = {
                "avatars": {
                    "min_avatar_height": this.minAvatarHeight,
                    "max_avatar_height": this.maxAvatarHeight,
                    "avatar_whitelist": this.avatarWhitelist,
                    "replacement_avatar": this.replacementAvatar
                }
            };
            Settings.automaticCommitSettings(settingsToCommit);
        }
    },
    computed: {
        minAvatarHeight: {
            get (): number {
                return this.values.avatars?.min_avatar_height ?? 0.4;
            },
            set (newMinAvatarHeight: number): void {
                if (typeof this.values.avatars?.min_avatar_height !== "undefined") {
                    this.values.avatars.min_avatar_height = newMinAvatarHeight;
                    this.saveSettings();
                }
            }
        },
        maxAvatarHeight: {
            get (): number {
                return this.values.avatars?.max_avatar_height ?? 5.2;
            },
            set (newMaxAvatarHeight: number): void {
                if (typeof this.values.avatars?.max_avatar_height !== "undefined") {
                    this.values.avatars.max_avatar_height = newMaxAvatarHeight;
                    this.saveSettings();
                }
            }
        },
        avatarWhitelist: {
            get (): string {
                return this.values.avatars?.avatar_whitelist ?? "";
            },
            set (newAvatarWhitelist: string): void {
                if (typeof this.values.avatars?.avatar_whitelist !== "undefined") {
                    this.values.avatars.avatar_whitelist = newAvatarWhitelist;
                    this.saveSettings();
                }
            }
        },
        replacementAvatar: {
            get (): string {
                return this.values.avatars?.replacement_avatar ?? "";
            },
            set (newReplacementAvatar: string): void {
                if (typeof this.values.avatars?.replacement_avatar !== "undefined") {
                    this.values.avatars.replacement_avatar = newReplacementAvatar;
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
