<!--
//  MetaverseLogin.vue
//
//  Created by Kalila L. on May 18th, 2021.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <q-form
        @submit="onSubmit"
        @reset="onReset"
        class="q-gutter-md"
        :autocomplete="AUTOCOMPLETE"
    >
        <q-input
            v-model="username"
            filled
            dark
            label="Username"
            hint="Enter your username."
            lazy-rules
            :rules="[ val => val && val.length > 0 || 'Please enter a username.']"
        />

        <q-input
            v-model="password"
            filled
            dark
            label="Password"
            :type="showPassword ? 'text' : 'password'"
            hint="Enter your password."
            lazy-rules
            :rules="[ val => val && val.length > 0 || 'Please enter a password.']"
        >
            <template v-slot:append>
                <q-icon
                    :name="showPassword ? 'visibility' : 'visibility_off'"
                    class="cursor-pointer"
                    @click="showPassword = !showPassword"
                />
            </template>
        </q-input>

        <div align="right">
            <q-btn label="Reset" type="reset" color="primary" flat class="q-mr-sm" />
            <q-btn label="Login" type="submit" color="primary"/>
        </div>
    </q-form>
</template>

<script>
// FIXME: Needs to be done correctly. Also universally? Maybe window.axios?
const axios = require("axios");

import Log from "../../../modules/utilities/log";

export default {
    name: "MetaverseLogin",

    emits: ["loginResult"],

    data: () => ({
        username: "",
        password: "",
        showPassword: false,
        // TODO: Needs to be stored somewhere central.
        DEFAULT_METAVERSE_URL: "https://metaverse.vircadia.com/live",
        AUTOCOMPLETE: false
    }),

    methods: {
        async onSubmit () {
            const metaverseUrl = await this.retrieveMetaverseUrl();
            const result = await this.attemptLogin(metaverseUrl, this.username, this.password);

            this.$emit("loginResult", { "success": result.success, "metaverse": metaverseUrl, "data": result.response });
        },

        // TODO: This needs to be addressed in a more modular fashion to reuse and save state across multiple components.
        async retrieveMetaverseUrl () {
            return new Promise((resolve) => {
                axios.get("/api/metaverse_info")
                    .then((response) => {
                        Log.info(Log.types.METAVERSE, `Retrieved Metaverse URL ${response.data.metaverse_url}.`);
                        resolve(response.data.metaverse_url);
                    }, (error) => {
                        Log.error(Log.types.METAVERSE, `Failed to retrieve Metaverse URL, using default URL ${this.DEFAULT_METAVERSE_URL} instead. Error: ${error}`);
                        resolve(this.DEFAULT_METAVERSE_URL);
                    });
            });
        },

        async attemptLogin (metaverse, username, password) {
            Log.info(Log.types.METAVERSE, `Attempting to login as ${username}.`);

            return new Promise((resolve) => {
                axios.post(`${metaverse}/oauth/token`, {
                    grant_type: "password",
                    scope: "owner", // as opposed to 'domain', we're asking for a user token
                    username: username,
                    password: password
                })
                    .then((response) => {
                        Log.info(Log.types.METAVERSE, `Successfully got key and details for ${username}.`);
                        resolve({ "success": true, "response": response.data });
                    }, (error) => {
                        Log.error(Log.types.METAVERSE, `Failed to get key and details for ${username}.`);
                        if (error.response && error.response.data) {
                            resolve({ "success": false, "response": error.response.data });
                        } else if (error) {
                            resolve({ "success": false, "response": error });
                        } else {
                            resolve({ "success": false, "response": "Unknown reason." });
                        }
                    });
            });
        },

        onReset () {
            this.username = "";
            this.password = "";
        }
    }
};
</script>
