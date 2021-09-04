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
                                        size="sm"
                                        flat
                                    >
                                        Skip Wizard
                                    </q-btn>
                                </q-card-actions>
                            </q-card>
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
                                        @click="$refs.stepper.next()"
                                        class="q-mb-md"
                                        size="md"
                                        outline
                                        text-color="white"
                                        icon-right="chevron_right"
                                    >
                                        Continue
                                    </q-btn>
                                    <q-btn
                                        class="q-mb-md"
                                        size="sm"
                                        outline
                                        disabled
                                        text-color="white"
                                        icon-right="upload"
                                    >
                                        Import (coming soon)
                                    </q-btn>
                                    <q-btn
                                        @click="$refs.stepper.previous()"
                                        size="sm"
                                        flat
                                        icon-left="chevron_left"
                                    >
                                        Back
                                    </q-btn>
                                </q-card-actions>
                            </q-card>
                        </q-step>

                        <q-step
                            :name="4"
                            title="Metaverse"
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
                                        outline
                                        text-color="white"
                                        icon-right="cloud"
                                    >
                                        Connect
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
                                        icon-left="chevron_left"
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
import { defineComponent, ref } from 'vue';

export default defineComponent({
    name: 'Index',
    data () {
        return {
            mainOverlay: true,
            firstTimeWizardDialog: true,
            welcomeText: true,
            mainWizard: true,
            mainWizardStep: ref(1),
            // Consts
            WELCOME_TEXT_TIMEOUT: 2000,
            MAIN_WIZARD_TRANSITION_TIME: 1000,
            DEFAULT_METAVERSE_URL: "https://metaverse.vircadia.com/live"
        };
    },
    mounted () {
        setTimeout(() => {
            this.mainWizardStep++;
        }, this.WELCOME_TEXT_TIMEOUT);
    },
    methods: {
        connectMetaverseTriggered () {
            const axios = require('axios');

            axios.get('/api/metaverse')
                .then(function (response) {
                    // handle success
                    console.log(response);
                })
                .catch(function (error) {
                    // handle error
                    console.log(error);
                })
                .then(function () {
                    // always executed
                });
        }
    }
});
</script>
