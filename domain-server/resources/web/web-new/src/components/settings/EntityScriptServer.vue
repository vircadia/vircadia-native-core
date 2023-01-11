<template>
    <div>
        <!-- Entities Script Server Settings -->
        <q-card class="my-card q-mt-md">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold q-mb-sm">Entity Script Server (ESS)</div>
                <q-separator />
                <!-- ADVANCED SETTINGS SECTION -->
                <q-expansion-item v-model="isWordPressSettingsToggled" class="q-mt-md text-subtitle1" popup icon="settings" label="Advanced Settings">
                    <q-card>
                        <!-- Entity PPS per script section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="entityPPSPerScript" label="Entity PPS per script" type="number"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">The number of packets per second (PPS) that can be sent to the entity server for each server entity script. This contributes to a total overall amount.
                                <br/>Example: 1000 PPS with 5 entites gives a total PPS of 5000 that is shared among the entity scripts. A single could use 4000 PPS, leaving 1000 for the rest, for example.</div>
                        </q-card-section>
                        <!-- Maximum Total Entity PPS section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="maxEntityPPS" label="Maximum Total Entity PPS" type="number"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">The maximum total packets per seconds (PPS) that can be sent to the entity server.
                                <br/>Example: 5 scripts @ 1000 PPS per script = 5000 total PPS. A maximum total PPS of 4000 would cap this 5000 total PPS to 4000.</div>
                        </q-card-section>
                    </q-card>
                </q-expansion-item>
                <!-- *END* ADVANCED SETTINGS SECTION *END* -->
            </q-card-section>
        </q-card>
        <!-- *END* Entities Script Server Settings *END* -->
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { Settings } from "@Modules/domain/settings";
import { EntityScriptServerSaveSettings, SettingsValues } from "@/src/modules/domain/interfaces/settings";

export default defineComponent({
    name: "EntityScriptServerSettings",

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
            const settingsToCommit: EntityScriptServerSaveSettings = {
                "entity_script_server": {
                    "entity_pps_per_script": this.entityPPSPerScript,
                    "max_total_entity_pps": this.maxEntityPPS
                }
            };
            Settings.automaticCommitSettings(settingsToCommit);
        }
    },
    computed: {
        entityPPSPerScript: {
            get (): number {
                return this.values.entity_script_server?.entity_pps_per_script ?? 0;
            },
            set (newEntityPPS: number): void {
                if (typeof this.values.entity_script_server?.entity_pps_per_script !== "undefined") {
                    this.values.entity_script_server.entity_pps_per_script = newEntityPPS;
                    this.saveSettings();
                }
            }
        },
        maxEntityPPS: {
            get (): number {
                return this.values.entity_script_server?.max_total_entity_pps ?? 0;
            },
            set (newMaxEntityPPS: number): void {
                if (typeof this.values.entity_script_server?.max_total_entity_pps !== "undefined") {
                    this.values.entity_script_server.max_total_entity_pps = newMaxEntityPPS;
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
