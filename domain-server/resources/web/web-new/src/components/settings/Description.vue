<template>
    <div>
        <!-- Description Settings -->
        <q-card class="my-card">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold q-mb-sm">Description</div>
                <q-separator />
                <!-- ADVANCED SETTINGS SECTION -->
                <q-expansion-item v-model="isWordPressSettingsToggled" class="q-mt-md text-subtitle1" popup icon="settings" label="Advanced Settings">
                    <q-card>
                        <!-- Name Section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="worldName" label="Name"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">The name of your domain (256 character limit).</div>
                        </q-card-section>
                        <!-- Description Section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="description" label="Description"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">A description of your domain (256 character limit).</div>
                        </q-card-section>
                        <!-- World Thumbnail Section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="thumbnail" label="World Thumbnail"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">A link to the thumbnail that is publicly accessible from the internet.</div>
                        </q-card-section>
                        <!-- World Images section -->
                        <q-card-section>
                            <p class="q-mb-xs text-body1 text-weight-bold">World Images</p>
                            <div class="q-mt-xs text-caption text-grey-5">URLs to images that visually describe your world to potential visitors.</div>
                            <q-table dark class="bg-grey-9" :rows="rows">
                                <template v-slot:header>
                                    <q-tr class="bg-primary">
                                        <q-th class="text-left">Image URL</q-th>
                                        <q-th auto-width></q-th> <!-- Empty column for delete buttons -->
                                    </q-tr>
                                </template>
                                <template v-slot:body>
                                    <q-tr v-for="(imageURL, index) in images" :key="imageURL">
                                        <q-td>{{ imageURL }}</q-td>
                                        <q-td class="text-center"><q-btn size="sm" color="negative" icon="delete" class="q-px-xs" @click="onShowConfirmDeleteDialogue('images', index, imageURL)" round /></q-td>
                                    </q-tr>
                                </template>
                                <template v-slot:bottom-row>
                                    <q-tr>
                                        <q-td>
                                            <q-input v-model="newRowNames.images" class="no-margin no-padding text-subtitle2 text-white" standout="bg-primary text-white" label="New Image URL" dense/>
                                        </q-td>
                                        <q-td class="text-center">
                                            <q-btn @click="onAddRow('images')" color="positive"><q-icon name="add" size="sm"/></q-btn>
                                        </q-td>
                                    </q-tr>
                                </template>
                            </q-table>
                        </q-card-section>
                        <!-- Maturity Rating section -->
                        <q-card-section>
                            <q-select standout="bg-primary text-white" color="primary" emit-value map-options v-model="maturity" :options="maturityOptions" label="Maturity Rating" transition-show="jump-up" transition-hide="jump-down">
                                <template v-slot:prepend>
                                    <q-icon name="diversity_3" />
                                </template>
                            </q-select>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">A maturity rating, available as a guideline for content on your domain.</div>
                        </q-card-section>
                        <!-- World Administrative Contact Section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="contactInfo" label="World Administrative Contact"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Contact information to reach server administrators for assistance (256 character limit).</div>
                        </q-card-section>
                        <!-- World Managers / Administrators section -->
                        <q-card-section>
                            <p class="q-mb-xs text-body1 text-weight-bold">World Managers / Administrators</p>
                            <div class="q-mt-xs text-caption text-grey-5">Usernames of managers that administrate the domain.</div>
                            <q-table dark class="bg-grey-9" :rows="rows">
                                <template v-slot:header>
                                    <q-tr class="bg-primary">
                                        <q-th class="text-left">Username</q-th>
                                        <q-th auto-width></q-th> <!-- Empty column for delete buttons -->
                                    </q-tr>
                                </template>
                                <template v-slot:body>
                                    <q-tr v-for="(managerUsername, index) in managers" :key="managerUsername">
                                        <q-td>{{ managerUsername }}</q-td>
                                        <q-td class="text-center"><q-btn size="sm" color="negative" icon="delete" class="q-px-xs" @click="onShowConfirmDeleteDialogue('managers', index, managerUsername)" round /></q-td>
                                    </q-tr>
                                </template>
                                <template v-slot:bottom-row>
                                    <q-tr>
                                        <q-td>
                                            <q-input v-model="newRowNames.managers" class="no-margin no-padding text-subtitle2 text-white" standout="bg-primary text-white" label="Add Manager/Admin Username" dense/>
                                        </q-td>
                                        <q-td class="text-center">
                                            <q-btn @click="onAddRow('managers')" color="positive"><q-icon name="add" size="sm"/></q-btn>
                                        </q-td>
                                    </q-tr>
                                </template>
                            </q-table>
                        </q-card-section>
                        <!-- Category Tags section -->
                        <q-card-section>
                            <p class="q-mb-xs text-body1 text-weight-bold">Category Tags</p>
                            <div class="q-mt-xs text-caption text-grey-5">Common categories under which your domain falls.</div>
                            <q-table dark class="bg-grey-9" :rows="rows">
                                <template v-slot:header>
                                    <q-tr class="bg-primary">
                                        <q-th class="text-left">Category Tag</q-th>
                                        <q-th auto-width></q-th> <!-- Empty column for delete buttons -->
                                    </q-tr>
                                </template>
                                <template v-slot:body>
                                    <q-tr v-for="(tag, index) in tags" :key="tag">
                                        <q-td>{{ tag }}</q-td>
                                        <q-td class="text-center"><q-btn size="sm" color="negative" icon="delete" class="q-px-xs" @click="onShowConfirmDeleteDialogue('tags', index, tag)" round /></q-td>
                                    </q-tr>
                                </template>
                                <template v-slot:bottom-row>
                                    <q-tr>
                                        <q-td>
                                            <q-input v-model="newRowNames.tags" class="no-margin no-padding text-subtitle2 text-white" standout="bg-primary text-white" label="Add a New Tag" dense/>
                                        </q-td>
                                        <q-td class="text-center">
                                            <q-btn @click="onAddRow('tags')" color="positive"><q-icon name="add" size="sm"/></q-btn>
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
                                <q-btn flat label="Delete" @click="onDeleteRow(confirmDeleteDialogue.settingType, confirmDeleteDialogue.index)"/>
                            </q-card-actions>
                        </q-card>
                    </q-dialog>
                </q-expansion-item>
                <!-- *END* ADVANCED SETTINGS SECTION *END* -->
            </q-card-section>
        </q-card>
        <!-- *END* Description Settings *END* -->
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { Settings } from "@Modules/domain/settings";
import { DescriptionSaveSettings, SettingsValues } from "@/src/modules/domain/interfaces/settings";

type settingTypes = "images" | "managers" | "tags";

export default defineComponent({
    name: "DescriptionSettings",

    data () {
        return {
            rows: [{}],
            isWordPressSettingsToggled: false,
            values: {} as SettingsValues,
            newRowNames: { images: "", managers: "", tags: "" },
            confirmDeleteDialogue: { show: false, thingToDelete: "", index: -1, settingType: "" as settingTypes },
            maturityOptions: [
                {
                    label: "Unrated",
                    value: "unrated"
                },
                {
                    label: "Everyone",
                    value: "everyone"
                },
                {
                    label: "Teen (13+)",
                    value: "teen"
                },
                {
                    label: "Mature (17+)",
                    value: "mature"
                },
                {
                    label: "Adult (18+)",
                    value: "adult"
                }
            ]
        };
    },
    methods: {
        onShowConfirmDeleteDialogue (settingType: settingTypes, index: number, thingToDelete: string): void {
            this.confirmDeleteDialogue = { show: true, thingToDelete: thingToDelete, index: index, settingType: settingType };
        },
        onHideConfirmDeleteDialogue (): void {
            this.confirmDeleteDialogue = { show: false, thingToDelete: "", index: -1, settingType: "" as settingTypes };
        },
        onDeleteRow (settingType: settingTypes, index: number): void {
            this.onHideConfirmDeleteDialogue();
            const changedSetting = [...this[settingType]];
            changedSetting.splice(index, 1);
            this[settingType] = [...changedSetting];
        },
        onAddRow (settingType: settingTypes) {
            const newRowSetting = [...this[settingType], this.newRowNames[settingType]];
            this[settingType] = [...newRowSetting];
            this.newRowNames[settingType] = "";
        },
        async refreshSettingsValues (): Promise<void> {
            this.values = await Settings.getValues();
        },
        saveSettings (): void {
            const settingsToCommit: DescriptionSaveSettings = {
                "descriptors": {
                    "world_name": this.worldName,
                    "description": this.description,
                    "thumbnail": this.thumbnail,
                    "images": this.images,
                    "maturity": this.maturity,
                    "contact_info": this.contactInfo,
                    "managers": this.managers,
                    "tags": this.tags
                }
            };
            Settings.automaticCommitSettings(settingsToCommit);
        }
    },
    computed: {
        worldName: {
            get (): string {
                return this.values.descriptors?.world_name ?? "error";
            },
            set (newWorldName: string): void {
                if (typeof this.values.descriptors?.world_name !== "undefined") {
                    this.values.descriptors.world_name = newWorldName;
                    this.saveSettings();
                }
            }
        },
        description: {
            get (): string {
                return this.values.descriptors?.description ?? "error";
            },
            set (newDescription: string): void {
                if (typeof this.values.descriptors?.description !== "undefined") {
                    this.values.descriptors.description = newDescription;
                    this.saveSettings();
                }
            }
        },
        thumbnail: {
            get (): string {
                return this.values.descriptors?.thumbnail ?? "error";
            },
            set (newThumbnail: string): void {
                if (typeof this.values.descriptors?.thumbnail !== "undefined") {
                    this.values.descriptors.thumbnail = newThumbnail;
                    this.saveSettings();
                }
            }
        },
        images: {
            get (): string[] {
                return this.values.descriptors?.images ?? [];
            },
            set (newImages: string[]): void {
                if (typeof this.values.descriptors?.images !== "undefined") {
                    this.values.descriptors.images = newImages;
                    this.saveSettings();
                }
            }
        },
        maturity: {
            get (): string {
                return this.values.descriptors?.maturity ?? "error";
            },
            set (newMaturity: string): void {
                if (typeof this.values.descriptors?.maturity !== "undefined") {
                    this.values.descriptors.maturity = newMaturity;
                    this.saveSettings();
                }
            }
        },
        contactInfo: {
            get (): string {
                return this.values.descriptors?.contact_info ?? "error";
            },
            set (newContactInfo: string): void {
                if (typeof this.values.descriptors?.contact_info !== "undefined") {
                    this.values.descriptors.contact_info = newContactInfo;
                    this.saveSettings();
                }
            }
        },
        managers: {
            get (): string[] {
                return this.values.descriptors?.managers ?? [];
            },
            set (newManagers: string[]): void {
                if (typeof this.values.descriptors?.managers !== "undefined") {
                    this.values.descriptors.managers = newManagers;
                    this.saveSettings();
                }
            }
        },
        tags: {
            get (): string[] {
                return this.values.descriptors?.tags ?? [];
            },
            set (newTags: string[]): void {
                if (typeof this.values.descriptors?.tags !== "undefined") {
                    this.values.descriptors.tags = newTags;
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
