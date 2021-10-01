<!--
//  Index.vue
//
//  Created by Kalila L. on Sep. 4th, 2021.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<style lang="scss" scoped>
    $firstTimeWizardContainerBGstart: rgba(0, 0, 0, 0.0);
    $firstTimeWizardContainerBGend: rgba(0, 0, 0, 0.75);

    #firstTimeWizardContainer {
        background-color: $firstTimeWizardContainerBGend;

        animation: firstTimeWizardContainerFadeIn 5s;
        -webkit-animation: firstTimeWizardContainerFadeIn 5s;
        -moz-animation: firstTimeWizardContainerFadeIn 5s;
        -o-animation: firstTimeWizardContainerFadeIn 5s;
        -ms-animation: firstTimeWizardContainerFadeIn 5s;
    }

    @keyframes firstTimeWizardContainerFadeIn {
      0% {background-color: $firstTimeWizardContainerBGstart;}
      100% {background-color: $firstTimeWizardContainerBGend}
    }

    @-moz-keyframes firstTimeWizardContainerFadeIn {
        0% {background-color: $firstTimeWizardContainerBGstart;}
        100% {background-color: $firstTimeWizardContainerBGend}
    }

    @-webkit-keyframes firstTimeWizardContainerFadeIn {
        0% {background-color: $firstTimeWizardContainerBGstart;}
        100% {background-color: $firstTimeWizardContainerBGend}
    }

    @-o-keyframes firstTimeWizardContainerFadeIn {
        0% {background-color: $firstTimeWizardContainerBGstart;}
        100% {background-color: $firstTimeWizardContainerBGend}
    }

    @-ms-keyframes firstTimeWizardContainerFadeIn {
        0% {background-color: $firstTimeWizardContainerBGstart;}
        100% {background-color: rgba(0, 0, 0, 0.5);}
    }

    .welcome {
        color: white;
    }

    .wizardCard {
        color: white;
        background: none !important;
        box-shadow: none !important;
    }

    .mainWizardStepper {
        background: none !important;
        box-shadow: none !important;
    }

    ::v-deep(.q-stepper__header) {
        display: none !important;
    }

    ::v-deep(.q-panel) {
        overflow: hidden !important;
    }

    .q-dialog__inner div {
        box-shadow: none !important;
    }

    .q-dialog__inner div {
        border: none !important;
    }
</style>

<template>
    <div
        id="firstTimeWizardContainer"
        class="window-height window-width"
    >
        <q-dialog
            v-model="firstTimeWizardDialog"
            class="overflow-hidden"
            persistent
            square
            seamless
        >
            <div class="overflow-hidden">
                <transition
                    name="mode-fade"
                    mode="out-in"
                    appear
                    enter-active-class="animated fadeIn"
                    leave-active-class="animated fadeOut"
                    :duration="MAIN_WIZARD_TRANSITION_TIME"
                >
                    <q-stepper
                        v-show="mainWizard"
                        v-model="mainWizardStep"
                        class="mainWizardStepper overflow-hidden"
                        ref="stepper"
                        :animated="true"
                        transition-prev="slide-right"
                        transition-next="slide-left"
                        style="width: 100%"
                    >
                        <q-step
                            :name="1"
                            title="Welcome"
                            :done="mainWizardStep > 1"
                        >
                            <h2
                                class="welcome text-weight-thin"
                            >
                                Welcome
                            </h2>
                        </q-step>

                        <q-step
                            :name="2"
                            title="Begin Wizard"
                            caption="Optional"
                            :done="mainWizardStep > 2"
                        >
                            <q-card
                                class="wizardCard"
                            >
                                <q-card-section>
                                    <div class="text-h7 text-weight-light text-center">❤</div>
                                    <div class="text-h6 text-weight-light text-center">Let's configure your virtual world.</div>
                                </q-card-section>

                                <q-card-actions vertical align="right">
                                    <q-btn
                                        @click="$refs.stepper.next()"
                                        class="q-mb-md"
                                        size="md"
                                        outline
                                        text-color="white"
                                        icon-right="chevron_right"
                                    >
                                        Start
                                    </q-btn>
                                    <q-btn
                                        @click="skipWizardDialog = true"
                                        size="sm"
                                        flat
                                    >
                                        Skip Wizard
                                    </q-btn>
                                </q-card-actions>
                            </q-card>

                            <q-dialog v-model="skipWizardDialog">
                                <q-card
                                    class="column no-wrap items-stretch q-pa-md"
                                    style="background: rgba(0, 0, 0, 0.95);"
                                    dark
                                >
                                <q-card-section>
                                    <div class="text-h6 text-weight-light text-center">Are you sure you want to skip the wizard?</div>
                                </q-card-section>

                                <q-card-actions align="right">
                                    <q-btn
                                        @click="skipWizardDialog = false"
                                    >
                                        No
                                    </q-btn>
                                    <q-btn
                                        @click="mainWizardStep = FINAL_WIZARD_STEP; skipWizardDialog = false;"
                                        size="md"
                                    >
                                        Skip
                                    </q-btn>
                                </q-card-actions>
                                </q-card>
                            </q-dialog>
                        </q-step>

                        <q-step
                            :name="3"
                            title="Import"
                            caption="Optional"
                            :done="mainWizardStep > 3"
                        >
                            <q-card
                                class="wizardCard"
                            >
                                <q-card-section>
                                    <div class="text-h6 text-weight-light text-center">Import settings and/or content?</div>
                                    <div class="text-h7 text-weight-light text-center">You can always do this later.</div>
                                </q-card-section>

                                <q-card-actions vertical align="right">
                                    <q-btn
                                        class="q-mb-md"
                                        size="md"
                                        outline
                                        disabled
                                        text-color="white"
                                        icon-right="upload"
                                    >
                                        Import (coming soon)
                                    </q-btn>
                                    <q-btn
                                        @click="$refs.stepper.next()"
                                        class="q-mb-md"
                                        size="sm"
                                        outline
                                        text-color="white"
                                        icon-right="chevron_right"
                                    >
                                        Skip
                                    </q-btn>
                                    <q-btn
                                        @click="$refs.stepper.previous()"
                                        size="sm"
                                        flat
                                        icon="chevron_left"
                                    >
                                        Back
                                    </q-btn>
                                </q-card-actions>
                            </q-card>
                        </q-step>

                        <q-step
                            :name="4"
                            :title="connectMetaverseSuccess ? 'Metaverse (Connected ✔️)' : 'Metaverse'"
                            caption="Optional"
                            :done="mainWizardStep > 4"
                        >
                            <q-card
                                class="wizardCard"
                            >
                                <q-card-section>
                                    <div class="text-h6 text-weight-light text-center">Connect your world to your Metaverse account?</div>
                                    <div class="text-h7 text-weight-light text-center">This can improve security and discovery for your world.</div>
                                </q-card-section>

                                <q-card-actions vertical align="right">
                                    <q-btn
                                        @click="connectMetaverseTriggered"
                                        class="q-mb-md"
                                        size="md"
                                        :outline="!connectMetaverseSuccess"
                                        :color="connectMetaverseSuccess ? 'green' : ''"
                                        text-color="white"
                                        :icon-right="connectMetaverseSuccess ? 'done' : 'cloud'"
                                    >
                                        {{ connectMetaverseSuccess ? 'Connected' : 'Connect' }}
                                    </q-btn>
                                    <q-btn
                                        @click="connectMetaverseSuccess ? $refs.stepper.next() : mainWizardStep = 6"
                                        class="q-mb-md"
                                        :size="connectMetaverseSuccess ? 'md' : 'sm'"
                                        outline
                                        text-color="white"
                                        icon-right="chevron_right"
                                    >
                                        {{ connectMetaverseSuccess ? 'Next' : 'Skip' }}
                                    </q-btn>
                                    <q-btn
                                        @click="$refs.stepper.previous()"
                                        size="sm"
                                        flat
                                        icon="chevron_left"
                                    >
                                        Back
                                    </q-btn>
                                </q-card-actions>
                            </q-card>

                            <q-dialog v-model="connectMetaverseDialog">
                                <q-card
                                    class="column no-wrap items-stretch q-pa-md"
                                    style="background: rgba(0, 0, 0, 0.95);"
                                    dark
                                >
                                    <ConnectMetaverse @connectionResult="onMetaverseConnectionAttempted"></ConnectMetaverse>
                                </q-card>
                            </q-dialog>
                        </q-step>

                        <q-step
                            :name="5"
                            title="Configure Metaverse"
                            caption="Recommended"
                            :done="mainWizardStep > 5"
                        >
                            <q-card
                                class="wizardCard"
                            >
                                <q-card-section>
                                    <div class="text-h6 text-weight-light text-center">Let's give your Domain a label.</div>
                                    <div class="text-h7 text-weight-light text-center">This is to help you identify your Domains in your Metaverse account.</div>
                                </q-card-section>

                                <q-card-section>
                                    <q-input
                                        v-model="domainLabel"
                                        filled
                                        dark
                                        label="Label"
                                        hint="Enter a nickname for your Domain."
                                        lazy-rules
                                        :rules="[ val => val && val.length > 0 || 'Please enter a label.']"
                                    />
                                </q-card-section>

                                <q-card-actions vertical align="right">
                                    <q-btn
                                        @click="saveMetaverseConfiguration"
                                        class="q-mb-md"
                                        size="md"
                                        outline
                                        text-color="white"
                                        icon-right="chevron_right"
                                    >
                                        Next
                                    </q-btn>
                                    <q-btn
                                        @click="$refs.stepper.previous()"
                                        size="sm"
                                        flat
                                        icon="chevron_left"
                                    >
                                        Back
                                    </q-btn>
                                </q-card-actions>
                            </q-card>
                        </q-step>

                        <q-step
                            :name="6"
                            title="Access"
                            caption="Recommended"
                            :done="mainWizardStep > 6"
                        >
                            <q-card
                                class="wizardCard"
                            >
                                <q-card-section>
                                    <div class="text-h6 text-weight-light text-center">Let's configure some security settings for your world.</div>
                                </q-card-section>

                                <q-card-section v-show="connectMetaverseSuccess">
                                    <div class="text-h6 text-weight-light text-center">Who should be an <b>in-world admin</b> of your Domain?</div>
                                    <q-select
                                        label="Metaverse usernames (press enter)"
                                        filled
                                        dark
                                        v-model="administratorsListSecurityModel"
                                        use-input
                                        use-chips
                                        multiple
                                        hide-dropdown-icon
                                        input-debounce="0"
                                        new-value-mode="add-unique"
                                    />
                                </q-card-section>

                                <q-card-section>
                                    <div class="text-h6 text-weight-light text-center">Who can <b>connect</b> to your Domain?</div>
                                    <q-option-group
                                        v-model="connectionSecurityModel"
                                        :options="connectionSecurityOptions"
                                        color="primary"
                                        dark
                                        type="checkbox"
                                    />
                                </q-card-section>

                                <q-card-section>
                                    <div class="text-h6 text-weight-light text-center">Who can <b>rez</b> items in your Domain?</div>
                                    <q-option-group
                                        v-model="rezSecurityModel"
                                        :options="rezSecurityOptions"
                                        color="primary"
                                        dark
                                        type="checkbox"
                                    />
                                </q-card-section>

                                <q-card-actions vertical align="right">
                                    <q-btn
                                        @click="saveSecuritySettings"
                                        class="q-mb-md"
                                        size="md"
                                        outline
                                        text-color="white"
                                        icon-right="chevron_right"
                                    >
                                        Next
                                    </q-btn>
                                    <q-btn
                                        @click="connectMetaverseSuccess ? $refs.stepper.previous() : mainWizardStep = 4"
                                        size="sm"
                                        flat
                                        icon="chevron_left"
                                    >
                                        Back
                                    </q-btn>
                                </q-card-actions>
                            </q-card>
                        </q-step>

                        <q-step
                            :name="7"
                            title="Administrator"
                            caption="Highly Recommended"
                            :done="mainWizardStep > 7"
                        >
                            <q-card
                                class="wizardCard"
                            >
                                <q-card-section>
                                    <div class="text-h6 text-weight-light text-center">Let's create a username and password for your Domain's administrator web panel.</div>
                                    <div class="text-h7 q-mt-sm text-weight-light text-red text-center">Warning: Immediately after saving the credentials, you will be prompted to enter them into your browser to continue.</div>
                                </q-card-section>

                                <q-card-section>
                                    <q-form
                                        @submit="saveAdminCredentialsDialog = true"
                                        @reset="onAdminStepReset"
                                        class="q-gutter-md"
                                    >
                                        <q-input
                                            v-model="adminCredentialsUsername"
                                            filled
                                            dark
                                            label="Username"
                                            hint="Enter your username."
                                            lazy-rules
                                            :rules="[ val => val && val.length > 0 || 'Please enter a username.']"
                                        />

                                        <q-input
                                            v-model="adminCredentialsPassword"
                                            filled
                                            dark
                                            label="Password"
                                            :type="adminCredentialsShowPassword ? 'text' : 'password'"
                                            hint="Enter your password."
                                            lazy-rules
                                            :rules="[ val => val && val.length > 0 || 'Please enter a password.']"
                                        >
                                            <template v-slot:append>
                                                <q-icon
                                                    :name="adminCredentialsShowPassword ? 'visibility' : 'visibility_off'"
                                                    class="cursor-pointer"
                                                    @click="adminCredentialsShowPassword = !adminCredentialsShowPassword"
                                                />
                                            </template>
                                        </q-input>

                                        <q-input
                                            v-model="adminCredentialsConfirmPassword"
                                            filled
                                            dark
                                            label="Confirm Password"
                                            :type="adminCredentialsShowConfirmPassword ? 'text' : 'password'"
                                            hint="Enter your password again."
                                            lazy-rules
                                            :rules="[ val => val && val.length > 0 && val === adminCredentialsPassword || 'Please ensure your passwords match.']"
                                        >
                                            <template v-slot:append>
                                                <q-icon
                                                    :name="adminCredentialsShowConfirmPassword ? 'visibility' : 'visibility_off'"
                                                    class="cursor-pointer"
                                                    @click="adminCredentialsShowConfirmPassword = !adminCredentialsShowConfirmPassword"
                                                />
                                            </template>
                                        </q-input>

                                        <div align="right">
                                            <q-btn
                                                label="Clear"
                                                type="reset"
                                                size="md"
                                                class="q-mb-md q-mr-sm"
                                                flat
                                            />
                                            <q-btn
                                                :label="adminCredentialsSaved ? 'Saved' : 'Save'"
                                                type="submit"
                                                class="q-mb-md"
                                                size="md"
                                                text-color="white"
                                                :outline="!adminCredentialsSaved"
                                                :color="adminCredentialsSaved ? 'green' : ''"
                                                :icon-right="adminCredentialsSaved ? 'done' : 'key'"
                                            />
                                        </div>
                                    </q-form>
                                </q-card-section>
                                <q-card-actions vertical align="right">
                                    <q-btn
                                        @click="$refs.stepper.next()"
                                        class="q-mb-md"
                                        :label="adminCredentialsSaved ? 'Next' : 'Skip'"
                                        :size="adminCredentialsSaved ? 'md' : 'sm'"
                                        outline
                                        text-color="white"
                                        icon-right="chevron_right"
                                    />
                                    <q-btn
                                        @click="$refs.stepper.previous()"
                                        size="sm"
                                        flat
                                        icon="chevron_left"
                                    >
                                        Back
                                    </q-btn>
                                </q-card-actions>
                            </q-card>

                            <q-dialog v-model="saveAdminCredentialsDialog">
                                <q-card
                                    class="column no-wrap items-stretch q-pa-md"
                                    style="background: rgba(0, 0, 0, 0.95);"
                                    dark
                                >
                                <q-card-section>
                                    <div class="text-h6 text-weight-light text-center">Are you sure you want to save these credentials?</div>
                                </q-card-section>

                                <q-card-actions align="right">
                                    <q-btn
                                        @click="saveAdminCredentialsDialog = false"
                                    >
                                        No
                                    </q-btn>
                                    <q-btn
                                        @click="onAdminStepSubmit(); saveAdminCredentialsDialog = false;"
                                        size="md"
                                    >
                                        Save
                                    </q-btn>
                                </q-card-actions>
                                </q-card>
                            </q-dialog>
                        </q-step>

                        <q-step
                            :name="8"
                            title="Performance"
                            caption=""
                            :done="mainWizardStep > 8"
                        >
                            <q-card
                                class="wizardCard"
                            >
                                <q-card-section>
                                    <div class="text-h6 text-weight-light text-center">Enable high performance mode?</div>
                                    <div class="text-h7 text-weight-light text-center">If you are hosting this Domain on your local computer and it is not very powerful, then consider leaving this off because the server will use more resources if it's busy, thereby slowing down your computer.
                                    If you are running this server on a powerful system (or a remote server) and intend to have a large audience, then turn this setting on.</div>
                                </q-card-section>

                                <q-card-section>
                                    <q-toggle
                                        v-model="performanceMode"
                                        checked-icon="check"
                                        color="red"
                                        label="Performance Mode"
                                        unchecked-icon="clear"
                                    />
                                </q-card-section>

                                <q-card-actions vertical align="right">
                                    <q-btn
                                        @click="$refs.stepper.next()"
                                        class="q-mb-md"
                                        size="md"
                                        outline
                                        text-color="white"
                                        icon-right="chevron_right"
                                    >
                                        Next
                                    </q-btn>
                                    <q-btn
                                        @click="$refs.stepper.previous()"
                                        size="sm"
                                        flat
                                        icon="chevron_left"
                                    >
                                        Back
                                    </q-btn>
                                </q-card-actions>
                            </q-card>
                        </q-step>

                        <q-step
                            :name="9"
                            title="Done!"
                            caption=""
                            :done="mainWizardStep > 9"
                        >
                            <q-card
                                class="wizardCard"
                            >
                                <q-card-section>
                                    <div class="text-h6 text-weight-light text-center">👍</div>
                                    <div class="text-h6 text-weight-light text-center">All done! Let's get you on your way.</div>
                                </q-card-section>

                                <q-card-actions vertical align="right">
                                    <q-btn
                                        @click="completeWizard"
                                        class="q-mb-md"
                                        size="md"
                                        outline
                                        text-color="white"
                                        icon-right="chevron_right"
                                    >
                                        Complete
                                    </q-btn>
                                    <q-btn
                                        @click="$refs.stepper.previous()"
                                        size="sm"
                                        flat
                                        icon="chevron_left"
                                    >
                                        Back
                                    </q-btn>
                                </q-card-actions>
                            </q-card>
                        </q-step>
                    </q-stepper>
                </transition>
            </div>
        </q-dialog>
    </div>
</template>

<script>
// FIXME: Needs to be done correctly. Also universally? Maybe window.axios?
const axios = require("axios");
const sha256 = require("js-sha256");

import { defineComponent, ref } from "vue";

// Components
import ConnectMetaverse from "../../components/dialogs/ConnectMetaverse.vue";
// Modules
import Log from "../../modules/utilities/log";

export default defineComponent({
    name: "Index",

    components: {
        ConnectMetaverse
    },

    data () {
        return {
            mainOverlay: true,
            firstTimeWizardDialog: true,
            welcomeText: true,
            mainWizard: true,
            mainWizardStep: ref(1),
            // Skip Wizard Dialog
            skipWizardDialog: false,
            // TODO: Needs to be based off of the actual state of the server's connection (using Vuex and retrieving settings on page load.)
            connectMetaverseSuccess: false,
            connectMetaverseDialog: false,
            // Metaverse Configuration
            domainLabel: "",
            // Security Step
            administratorsListSecurityModel: [],
            connectionSecurityModel: ref(["everyone"]),
            connectionSecurityOptions: [
                {
                    label: "Friends",
                    value: "friends"
                },
                {
                    label: "Logged In",
                    value: "logged-in"
                },
                {
                    label: "Everyone (anonymous)",
                    value: "everyone"
                }
            ],
            rezSecurityModel: ref(["friends"]),
            rezSecurityOptions: [
                {
                    label: "Friends",
                    value: "friends"
                },
                {
                    label: "Logged In",
                    value: "logged-in"
                },
                {
                    label: "Everyone (anonymous)",
                    value: "everyone"
                }
            ],
            // Administrator
            adminCredentialsUsername: "",
            adminCredentialsPassword: "",
            adminCredentialsConfirmPassword: "",
            adminCredentialsShowPassword: false,
            adminCredentialsShowConfirmPassword: false,
            adminCredentialsSaved: false,
            saveAdminCredentialsDialog: false,
            // Performance
            performanceMode: true,
            // Starter Content
            starterContentToggle: true,
            // Consts
            WELCOME_TEXT_TIMEOUT: 4500,
            MAIN_WIZARD_TRANSITION_TIME: 1000,
            DEFAULT_METAVERSE_URL: "https://metaverse.vircadia.com/live",
            FINAL_WIZARD_STEP: 9,
            COMPLETE_WIZARD_REDIRECT_DELAY: 4000
        };
    },

    mounted () {
        setTimeout(() => {
            this.mainWizardStep++;
        }, this.WELCOME_TEXT_TIMEOUT);
    },

    methods: {
        connectMetaverseTriggered () {
            this.connectMetaverseDialog = true;
        },

        onMetaverseConnectionAttempted (result) {
            this.connectMetaverseDialog = false;

            if (result.success === true) {
                this.connectMetaverseSuccess = true;
                this.mainWizardStep++;
            }
        },

        saveMetaverseConfiguration () {
            this.mainWizardStep++;
            // TODO: Put this path in a constant somewhere.
            axios.post("/api/domains", { "domain": { "label": this.domainLabel } },
                {
                    params: {
                        label: this.domainLabel
                    },
                    headers: {
                        "content-type": "application/x-www-form-urlencoded;charset=utf-8"
                    }
                }
            )
                .then((response) => {
                    Log.info(Log.types.METAVERSE, "Successfully configured Domain with Metaverse.");
                    console.info("received", response);
                    const settingsToCommit = {
                        "metaverse": {
                            "automatic_networking": "full",
                            "id": response.data.domain.domainId
                        },
                        "descriptors": {
                            "world_name": this.domainLabel
                        }
                    };

                    this.commitMetaverseConfig(settingsToCommit);
                })
                .catch((error) => {
                    Log.error(Log.types.METAVERSE, `Failed to configure Domain with Metaverse: ${error}`);
                    this.$q.notify({
                        type: "negative",
                        textColor: "white",
                        icon: "warning",
                        message: `Failed to label your Domain on the Metaverse: ${error}`
                    });
                });
        },

        async commitMetaverseConfig (jsonToCommit) {
            const committed = await this.commitSettings(jsonToCommit);

            if (committed === true) {
                Log.info(Log.types.METAVERSE, "Successfully committed Domain server config for the Metaverse.");
                this.$q.notify({
                    type: "positive",
                    textColor: "white",
                    icon: "cloud_done",
                    message: "Successfully labeled your Domain on your Metaverse account."
                });
            } else {
                Log.error(Log.types.METAVERSE, "Failed to configure server with Metaverse: Could not commit config to settings.");
                this.$q.notify({
                    type: "negative",
                    textColor: "white",
                    icon: "warning",
                    message: "Domain label with Metaverse attempt failed because the settings were unable to be saved."
                });
            }
        },

        async saveSecuritySettings () {
            const friendsCanConnect = this.connectionSecurityModel.includes("friends");
            const friendsCanRez = this.rezSecurityModel.includes("friends");

            const loggedInCanConnect = this.connectionSecurityModel.includes("logged-in");
            const loggedInCanRez = this.rezSecurityModel.includes("logged-in");

            const everyoneCanConnect = this.connectionSecurityModel.includes("everyone");
            const everyoneCanRez = this.rezSecurityModel.includes("everyone");

            // TODO: revisit methodology here, cloud domains don't need certain overrides for example.
            // However, local domain owners would.
            const localhostPermissions = true;

            const administratorsPermissions = [];

            this.administratorsListSecurityModel.forEach((adminUsername) => {
                administratorsPermissions.push({
                    "id_can_adjust_locks": true,
                    "id_can_connect": true,
                    "id_can_connect_past_max_capacity": true,
                    "id_can_get_and_set_private_user_data": true,
                    "id_can_kick": true,
                    "id_can_replace_content": true,
                    "id_can_rez": true,
                    "id_can_rez_avatar_entities": true,
                    "id_can_rez_certified": true,
                    "id_can_rez_tmp": true,
                    "id_can_rez_tmp_certified": true,
                    "id_can_write_to_asset_server": true,
                    "permissions_id": adminUsername
                });
            });

            const settingsToCommit = {
                "security": {
                    "permissions": administratorsPermissions,
                    "standard_permissions": [
                        {
                            "id_can_connect": everyoneCanConnect,
                            "id_can_rez_avatar_entities": everyoneCanConnect,
                            "id_can_rez": everyoneCanRez,
                            "id_can_rez_certified": everyoneCanRez,
                            "id_can_rez_tmp": everyoneCanRez,
                            "id_can_rez_tmp_certified": everyoneCanRez,
                            "permissions_id": "anonymous"
                        },
                        {
                            "id_can_connect": friendsCanConnect,
                            "id_can_rez_avatar_entities": friendsCanConnect,
                            "id_can_rez": friendsCanRez,
                            "id_can_rez_certified": friendsCanRez,
                            "id_can_rez_tmp": friendsCanRez,
                            "id_can_rez_tmp_certified": friendsCanRez,
                            "permissions_id": "friends"
                        },
                        {
                            "id_can_connect": loggedInCanConnect,
                            "id_can_rez_avatar_entities": loggedInCanConnect,
                            "id_can_rez": loggedInCanRez,
                            "id_can_rez_certified": loggedInCanRez,
                            "id_can_rez_tmp": loggedInCanRez,
                            "id_can_rez_tmp_certified": loggedInCanRez,
                            "permissions_id": "logged-in"
                        },
                        {
                            "id_can_adjust_locks": localhostPermissions,
                            "id_can_connect": localhostPermissions,
                            "id_can_rez_avatar_entities": localhostPermissions,
                            "id_can_connect_past_max_capacity": localhostPermissions,
                            "id_can_kick": localhostPermissions,
                            "id_can_replace_content": localhostPermissions,
                            "id_can_rez": localhostPermissions,
                            "id_can_rez_certified": localhostPermissions,
                            "id_can_rez_tmp": localhostPermissions,
                            "id_can_rez_tmp_certified": localhostPermissions,
                            "id_can_write_to_asset_server": localhostPermissions,
                            "permissions_id": "localhost"
                        }
                    ]
                }
            };

            const committed = await this.commitSettings(settingsToCommit);

            if (committed === true) {
                Log.info(Log.types.METAVERSE, "Successfully saved Domain server security settings.");
                this.$q.notify({
                    type: "positive",
                    textColor: "white",
                    icon: "cloud_done",
                    message: "Successfully saved your security settings."
                });
            } else {
                Log.error(Log.types.METAVERSE, "Failed to save Domain server security settings.");
                this.$q.notify({
                    type: "negative",
                    textColor: "white",
                    icon: "warning",
                    message: "Failed to save your security settings."
                });
            }

            // We move forward anyway, the user can come back if it fails or if they want to change it.
            this.mainWizardStep++;
        },

        async onAdminStepSubmit () {
            const settingsToCommit = {
                "security": {
                    "http_username": this.adminCredentialsUsername,
                    "http_password": sha256.hex(this.adminCredentialsConfirmPassword)
                }
            };

            const committed = await this.commitSettings(settingsToCommit);

            if (committed === true) {
                Log.info(Log.types.METAVERSE, "Successfully saved Domain server administrator details.");
                this.$q.notify({
                    type: "positive",
                    textColor: "white",
                    icon: "cloud_done",
                    message: "Successfully saved your administrator credentials."
                });

                this.adminCredentialsSaved = true;

                this.mainWizardStep++;
            } else {
                Log.error(Log.types.METAVERSE, "Failed to save Domain server administrator details.");
                this.$q.notify({
                    type: "negative",
                    textColor: "white",
                    icon: "warning",
                    message: "Failed to save your administrator credentials."
                });
            }
        },

        onAdminStepReset () {
            this.adminCredentialsUsername = "";
            this.adminCredentialsPassword = "";
            this.adminCredentialsConfirmPassword = "";
        },

        async completeWizard () {
            const settingsToCommit = {
                "wizard": {
                    "steps_completed": "0",
                    "completed_once": true
                }
            };

            const committed = await this.commitSettings(settingsToCommit);

            if (committed === true) {
                Log.info(Log.types.METAVERSE, "Successfully saved wizard completion.");
            } else {
                Log.error(Log.types.METAVERSE, "Failed to save wizard completion.");
            }

            this.redirectToSettings();
        },

        redirectToSettings () {
            // Needs to go somewhere universal.
            const redirectURL = "/settings" + location.search;

            this.firstTimeWizardDialog = false;

            this.$q.loading.show({
                message: "Stand by..."
            });

            setTimeout(() => {
                location.href = redirectURL;
            }, this.COMPLETE_WIZARD_REDIRECT_DELAY);
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

        // TODO: Needs to be somewhere universal. NOT TESTED.
        // uploadContentChunk (file, offset, id) {
        //     if (offset == undefined) {
        //         offset = 0;
        //     }
        //     if (id == undefined) {
        //         // Identify this upload session
        //         id = Math.round(Math.random() * 2147483647);
        //     }
        //
        //     const fileSize = file.size;
        //     const filename = file.name;
        //
        //     const CHUNK_SIZE = 1048576; // 1 MiB
        //
        //     const isFinal = Boolean(fileSize - offset <= CHUNK_SIZE);
        //     const nextChunkSize = Math.min(fileSize - offset, CHUNK_SIZE);
        //     const chunk = file.slice(offset, offset + nextChunkSize, file.type);
        //     const chunkFormData = new FormData();
        //
        //     let formItemName = "restore-file-chunk";
        //     if (offset == 0) {
        //         formItemName = isFinal ? "restore-file-chunk-only" : "restore-file-chunk-initial";
        //     } else if (isFinal) {
        //         formItemName = "restore-file-chunk-final";
        //     }
        //
        //     chunkFormData.append(formItemName, chunk, filename);
        //     const axiosParams = {
        //         url: "/content/upload",
        //         method: "POST",
        //         timeout: 30000, // 30 s
        //         headers: { "X-Session-Id": id },
        //         contentType: false,
        //         data: chunkFormData
        //     };
        //
        //     axios.post(axiosParams)
        //         .then(() => {
        //             Log.info(Log.types.DOMAIN, "Successfully uploaded content chunk.");
        //             if (!isFinal) {
        //                 this.uploadContentChunk(file, offset + CHUNK_SIZE, id);
        //             }
        //         })
        //         .catch((response) => {
        //             Log.error(Log.types.DOMAIN, `Failed to upload content chunk: ${response}`);
        //         });
        // }
    },

    watch: {
        async mainWizardStep () {
            const settingsToCommit = {
                "wizard": {
                    "steps_completed": this.mainWizardStep.toString()
                }
            };

            const committed = await this.commitSettings(settingsToCommit);

            if (committed === true) {
                Log.info(Log.types.DOMAIN, "Successfully committed steps completed to Domain server settings.");
            } else {
                Log.error(Log.types.DOMAIN, "Failed to commit steps completed to Domain server settings.");
            }
        },

        async performanceMode () {
            const settingsToCommit = {
                "audio_threading": {
                    "auto_threads": this.performanceMode
                },
                "avatar_mixer": {
                    "auto_threads": this.performanceMode
                }
            };

            const committed = await this.commitSettings(settingsToCommit);

            if (committed === true) {
                Log.info(Log.types.DOMAIN, "Successfully saved performance mode setting.");
                this.$q.notify({
                    type: "positive",
                    textColor: "white",
                    icon: "cloud_done",
                    message: "Successfully saved performance mode setting."
                });
            } else {
                Log.error(Log.types.DOMAIN, "Failed to save performance mode setting.");
                this.$q.notify({
                    type: "negative",
                    textColor: "white",
                    icon: "warning",
                    message: "Failed to save performance mode setting."
                });
            }
        },

        adminCredentialsUsername () {
            this.adminCredentialsSaved = false;
        },

        adminCredentialsPassword () {
            this.adminCredentialsSaved = false;
        },

        adminCredentialsConfirmPassword () {
            this.adminCredentialsSaved = false;
        }
    }
});
</script>
