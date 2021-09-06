<!--
//  ConnectMetaverse.vue
//
//  Created by Kalila L. on Sep. 5th, 2021.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <q-card-section>
        <div class="row no-wrap items-center">
            <div class="col text-h4 ellipsis">
                Metaverse
            </div>
        </div>

    </q-card-section>

    <q-separator />

    <q-card-section>
        <MetaverseLogin @loginResult="onLoginAttempt"></MetaverseLogin>
    </q-card-section>
</template>

<script>
// FIXME: Needs to be done correctly. Also universally? Maybe window.axios?
const axios = require("axios");

// Components
import MetaverseLogin from "../components/login/MetaverseLogin.vue";
// Modules
import Log from "../../modules/utilities/log";

export default {
    name: "ConnectMetaverse",

    components: {
        MetaverseLogin
    },

    emits: ["connectionResult"],

    data: () => ({
    }),

    methods: {
        onLoginAttempt (result) {
            if (result.success === true) {
                Log.info(Log.types.METAVERSE, `Successfully logged in as ${result.data.account_name} for Metaverse linking.`);
                // Get a token for our server from the Metaverse.
                axios.post(`${result.metaverse}/api/v1/token/new`, {}, {
                    params: {
                        // "asAdmin": store.account.useAsAdmin,
                        "scope": "domain"
                    },
                    headers: {
                        "Authorization": `Bearer ${result.data.access_token}`
                    }
                })
                    .catch((result) => {
                        Log.error(Log.types.METAVERSE, "Failed to link server with Metaverse.");

                        this.$q.notify({
                            type: "negative",
                            textColor: "white",
                            icon: "warning",
                            message: `Metaverse link attempt failed. ${result}`
                        });
                    })
                    .then(async (response) => {
                        Log.info(Log.types.METAVERSE, "Successfully got Domain token for Metaverse linking.");

                        const settingsToCommit = {
                            "metaverse": {
                                "access_token": response.data.data.token
                            }
                        };

                        const committed = await this.commitSettings(settingsToCommit);

                        if (committed === true) {
                            Log.info(Log.types.METAVERSE, "Successfully committed Domain server access token for the Metaverse.");
                            this.$q.notify({
                                type: "positive",
                                textColor: "white",
                                icon: "cloud_done",
                                message: "Successfully linked your server to the Metaverse."
                            });

                            this.$emit("connectionResult", { "success": true });
                        } else {
                            Log.error(Log.types.METAVERSE, "Failed to link server with Metaverse: Could not commit token to settings.");
                            this.$q.notify({
                                type: "negative",
                                textColor: "white",
                                icon: "warning",
                                message: "Metaverse link attempt failed because the settings were unable to be saved."
                            });

                            this.$emit("connectionResult", { "success": false });
                        }
                    });
            } else {
                this.$q.notify({
                    type: "negative",
                    textColor: "white",
                    icon: "warning",
                    message: `Login attempt failed: ${result.data.error}`
                });
            }
        },

        // TODO: This needs to go somewhere universal.
        commitSettings (settingsToCommit) {
            // TODO: This and all other URL endpoints should be in centralized (in their respective module) constants files.
            return axios.post("/settings.json", JSON.stringify(settingsToCommit))
                .then(() => {
                    Log.info(Log.types.DOMAIN, "Successfully committed settings.");
                    return true;
                })
                .catch((response) => {
                    Log.error(Log.types.DOMAIN, `Failed to commit settings to Domain: ${response}`);
                    return false;
                });
        }
    }
};
</script>
