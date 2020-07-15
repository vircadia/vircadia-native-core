<!--
//
//  Display.vue
//
//  Created by kasenvr@gmail.com on 13 Jul 2020
//  Copyright 2020 Vircadia and contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
-->

<template>
    <v-app id="inspire" max-height="100%">
        <!-- Main Slider Control Area -->
        <v-main>
            <v-container
                class="fill-height"
                fluid
            >
                <v-carousel 
                    v-model="currentSlide" 
                    class="fill-height"
                    :show-arrows="false"
                    height="100%"
                    :hide-delimiters="true"
                >
                    <v-carousel-item
                        v-for="(slide, index) in slides"
                        track-by="$index"
                        :key="index"
                    >
                        <v-img
                            :src="slide"
                            height="100%"
                            class="grey darken-4"
                            lazy-src="./assets/logo.png"
                        >
                            <template v-slot:placeholder>
                                <v-row
                                    class="fill-height ma-0"
                                    align="center"
                                    justify="center"
                                >
                                    <v-progress-circular size="128" width="16" indeterminate color="blue lighten-4"></v-progress-circular>
                                </v-row>
                            </template>
                        </v-img>
                    </v-carousel-item>
                </v-carousel>
            </v-container>
        </v-main>
        <!-- End Main Slider Control Area -->
        
        <!-- Change Presentation Channel Dialog -->
        
        <v-dialog v-model="changePresentationChannelDialogShow" persistent>
            <v-card>
                <v-toolbar>
                    <v-toolbar-title>Change Presentation Channel</v-toolbar-title>
                    <v-spacer></v-spacer>
                    <v-btn class="mx-2" color="red darken-1" @click="changePresentationChannelDialogShow = false">Close</v-btn>
                    <v-btn class="mx-2" color="green darken-1" @click="changePresentationChannelDialogShow = false; sendChannelUpdate()">Update</v-btn>
                </v-toolbar>

                <v-text-field
                    placeholder="Enter channel here"
                    v-model="changePresentationChannelDialogText"
                    filled
                ></v-text-field>
            </v-card>
        </v-dialog>
        
        <!-- Change Slide Channel Dialog -->
        
        <v-footer
            color="primary"
            app
            v-if="false"
        >
            <span class="">Presenting on channel: <b>{{ presentationChannel }}</b></span>
        </v-footer>
    </v-app>
</template>

<script>

var vue_this;

function browserDevelopment() {
    if (typeof EventBridge !== 'undefined') {
        return false; // We are in Vircadia.
    } else {
        return true; // We are in the browser, probably for development purposes.
    }
}

if (!browserDevelopment()) {
    // eslint-disable-next-line
    EventBridge.scriptEventReceived.connect(function(receivedCommand) {
        // console.log("receivedCommand:" + receivedCommand);

        receivedCommand = JSON.parse(receivedCommand);
        // alert("RECEIVED COMMAND:" + receivedCommand.command)
        if (receivedCommand.app === "slider-client-app") {
        // We route the data based on the command given.
            if (receivedCommand.command === 'script-to-web-initialize') {
                // console.log("INIT RECEIVED ON DISPLAY APP:" + JSON.stringify(receivedCommand.data));
                vue_this.initializeWebApp(receivedCommand.data);
            }
            
            if (receivedCommand.command === 'script-to-web-channel') {
                // alert("CHANNEL RECEIVED ON DISPLAY APP:" + JSON.stringify(receivedCommand.data));
                vue_this.receiveChannelUpdate(receivedCommand.data);
            }
            
            if (receivedCommand.command === 'script-to-web-display-slide') {
                // console.log("SLIDE RECEIVED ON DISPLAY APP:" + JSON.stringify(receivedCommand.data));
                vue_this.receiveSlides(receivedCommand.data);
            }
        }
    });
}


export default {
    props: {
        source: String,
    },
    data: () => ({
        drawer: null,
        slides: ['./assets/logo.png'], // Default slide on load...
        currentSlide: 0,
        presentationChannel: 'default-presentation-channel',
        // Change Presentation Channel Dialog
        changePresentationChannelDialogShow: false,
        changePresentationChannelDialogText: '',
    }),
    watch: {
    },
    methods: {
        initializeWebApp: function (data) {
            // The data should already be parsed.
            // console.log("DATA RECEIVED ON INIT:" + JSON.stringify(data));
            var parsedUserData = data.userData; 
            
            // if (parsedUserData.slides) {
            //     this.slides = parsedUserData.slides;
            // }
            
            if (parsedUserData.currentSlide) {
                vue_this.receiveSlides(parsedUserData.currentSlide);
            }

            if (parsedUserData.presentationChannel) {
                this.presentationChannel = parsedUserData.presentationChannel;
            }
        },
        receiveSlides: function (data) {
            this.slides.splice(0, this.slides.length, data);
            this.sendSync();
        },
        sendChannelUpdate: function () {
            this.presentationChannel = this.changePresentationChannelDialogText;
            this.changePresentationChannelDialogText = '';
            this.sendSync();
        },
        receiveChannelUpdate: function (data) {
            this.presentationChannel = data;
        },
        sendSync: function () {
            this.sendAppMessage("web-to-script-sync-state", { 
                "presentationChannel": this.presentationChannel,
                "currentSlide": this.slides[this.currentSlide]
            });
        },
        sendAppMessage: function(command, data) {
            var JSONtoSend = {
                "app": "slider-display-web",
                "command": command,
                "data": data
            };
            
            if (!browserDevelopment()) {
                // eslint-disable-next-line
                EventBridge.emitWebEvent(JSON.stringify(JSONtoSend));
            } else {
                alert(JSON.stringify(JSONtoSend));
            }
        }
    },
    created: function () {
        vue_this = this;
        
        this.sendAppMessage("ready", "");
    }
}
</script>