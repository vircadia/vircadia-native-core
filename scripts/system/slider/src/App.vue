<!--
//
//  App.vue
//
//  Created by kasenvr@gmail.com on 11 Jul 2020
//  Copyright 2020 Vircadia and contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
-->

<template>
    <v-app id="inspire">
        <v-navigation-drawer
            v-model="drawer"
            app
        >
            <v-list>
                <v-list-item link @click="addSlidesByURLDialogShow = !addSlidesByURLDialogShow">
                    <v-list-item-action>
                    <v-icon>mdi-plus</v-icon>
                    </v-list-item-action>
                    <v-list-item-content>
                        <v-list-item-title>Add Slide by URL</v-list-item-title>
                    </v-list-item-content>
                </v-list-item>
                <v-list-item link @click="uploadSlidesDialogShow = !uploadSlidesDialogShow">
                    <v-list-item-action>
                    <v-icon>mdi-upload</v-icon>
                    </v-list-item-action>
                    <v-list-item-content>
                        <v-list-item-title>Upload Slide</v-list-item-title>
                    </v-list-item-content>
                </v-list-item>
                <v-list-item link @click="manageSlidesDialogShow = !manageSlidesDialogShow">
                    <v-list-item-action>
                    <v-icon>mdi-pencil</v-icon>
                    </v-list-item-action>
                    <v-list-item-content>
                        <v-list-item-title>Manage Slides</v-list-item-title>
                    </v-list-item-content>
                </v-list-item>
                <v-list-item link @click="changePresentationChannelDialogShow = !changePresentationChannelDialogShow">
                    <v-list-item-action>
                    <v-icon>mdi-remote</v-icon>
                    </v-list-item-action>
                    <v-list-item-content>
                        <v-list-item-title>Presentation Channel</v-list-item-title>
                    </v-list-item-content>
                </v-list-item>
                <v-list-item link @click="changeSlideChannelDialogShow = !changeSlideChannelDialogShow">
                    <v-list-item-action>
                    <v-icon>mdi-database</v-icon>
                    </v-list-item-action>
                    <v-list-item-content>
                        <v-list-item-title>Slide Channel</v-list-item-title>
                    </v-list-item-content>
                </v-list-item>
            </v-list>
        </v-navigation-drawer>

        <v-app-bar
            app
            color="primary"
            dark
        >
            <v-app-bar-nav-icon @click.stop="drawer = !drawer"></v-app-bar-nav-icon>
            <v-toolbar-title>Presenter Panel</v-toolbar-title>
            <v-spacer></v-spacer>
            <div v-show="slides[slideChannel].length > 0">
                <v-btn medium fab @click="currentSlide--">
                    <v-icon>mdi-arrow-left</v-icon>
                </v-btn>
                <span class="mx-4">{{ currentSlide + 1 }} / {{ slides[slideChannel].length }}</span>
                <v-btn medium fab @click="currentSlide++">
                    <v-icon>mdi-arrow-right</v-icon>
                </v-btn>
            </div>
        </v-app-bar>
        
        <!-- Main Slider Control Area -->
        <v-main>
            <v-container
                class="fill-height"
                fluid
            >
                <v-carousel v-model="currentSlide" height="100%">
                    <v-carousel-item
                        v-for="(slide, index) in slides[slideChannel]"
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
        
        <!-- Add Slide by URL Dialog -->
        
        <v-dialog v-model="addSlidesByURLDialogShow" persistent>
            <v-card>
                <v-toolbar>
                    <v-toolbar-title>Add Slide by URL</v-toolbar-title>
                    <v-spacer></v-spacer>
                    <v-btn class="mx-2" color="red darken-1" @click="addSlidesByURLDialogShow = false">Close</v-btn>
                    <v-btn class="mx-2" color="green darken-1" @click="addSlidesByURLDialogShow = false; addSlideByURL()">Add</v-btn>
                </v-toolbar>

                <v-text-field
                    placeholder="Enter URL Here"
                    v-model="addSlideByURLField"
                    filled
                ></v-text-field>
            </v-card>
        </v-dialog>
        
        <!-- Add Slide by URL Dialog -->
        
        <!-- Add Slide by Upload Dialog -->
        
        <v-dialog v-model="uploadSlidesDialogShow" persistent>
            <v-card>
                <v-toolbar>
                    <v-toolbar-title>Upload Slide</v-toolbar-title>
                    <v-spacer></v-spacer>
                    <v-btn class="mx-2" color="red darken-1" @click="uploadSlidesDialogShow = false">Close</v-btn>
                    <v-btn class="mx-2" color="green darken-1" @click="uploadSlidesDialogShow = false; uploadSlide()">Upload</v-btn>
                </v-toolbar>
                <v-file-input
                    v-model="uploadSlidesDialogFiles"
                    :rules="uploadSlidesDialogRules"
                    :multiple="true"
                    accept="image/png, image/jpeg"
                    placeholder="Pick an image to upload as a slide"
                    prepend-icon="mdi-image-search"
                    label="Slide"
                ></v-file-input>
            </v-card>
        </v-dialog>
        
        <v-snackbar
            v-model="uploadSlidesFailedSnackbar"
            absolute
            centered
            color="red"
            elevation="24"
        >
            Upload Failed: {{ uploadSlidesFailedError }}
        </v-snackbar>
        
        <v-overlay
            opacity="0.9"
            v-model="uploadProcessingOverlay"
        >
            <v-btn
                icon
                @click="uploadProcessingOverlay = false"
            >
                <v-icon>mdi-close</v-icon>
            </v-btn>
            <br />
            <v-progress-circular indeterminate color="blue" size="64"></v-progress-circular>
        </v-overlay>
        
        <!-- Add Slide by Upload Dialog -->
        
        <!-- Manage Slides Dialog -->
        
        <v-dialog v-model="manageSlidesDialogShow" persistent>
            <v-card>
                <v-toolbar>
                    <v-toolbar-title>Manage Slides for {{ slideChannel }}</v-toolbar-title>
                    <v-spacer></v-spacer>
                    <v-btn class="mx-2" color="green" @click="manageSlidesDialogShow = false">Done</v-btn>
                </v-toolbar>
                <v-list subheader>
                    <v-subheader>{{ slides[slideChannel].length }} Slides</v-subheader>

                    <v-list-item
                        v-for="(slide, i) in slides[slideChannel]"
                        :key="slide"
                    >
                        <v-list-item-avatar size="64">
                            <v-img :src="slide"></v-img>
                        </v-list-item-avatar>

                        <v-list-item-content>
                            <v-list-item-subtitle>Slide {{i + 1}}</v-list-item-subtitle>
                            <v-list-item-title v-text="slide"></v-list-item-title>
                        </v-list-item-content>

                        <v-list-item-icon>
                            <v-btn :disabled="i === 0" @click="rearrangeSlide(i, 'up')" color="blue" class="mx-2" fab medium>
                                <v-icon>mdi-arrow-collapse-up</v-icon>
                            </v-btn>
                            <v-btn :disabled="i === slides.length - 1" @click="rearrangeSlide(i, 'down')" color="blue" class="mx-2" fab medium>
                                <v-icon>mdi-arrow-collapse-down</v-icon>
                            </v-btn>
                            <v-btn @click="confirmDeleteSlideDialogShow = true; confirmDeleteSlideDialogWhich = i" color="red" class="mx-2" fab medium>
                                <v-icon>mdi-delete</v-icon>
                            </v-btn>
                        </v-list-item-icon>
                    </v-list-item>
                </v-list>
            </v-card>
        </v-dialog>
        
        <!-- Manage Slides Dialog -->
        
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
                
                <v-footer>
                    <v-spacer></v-spacer>
                    <div>Current Channel: <b>{{ presentationChannel }}</b></div>
                </v-footer>
            </v-card>
        </v-dialog>
        
        <!-- Change Presentation Channel Dialog -->
        
        <!-- Change Slide Channel Dialog -->
        
        <v-dialog v-model="changeSlideChannelDialogShow" persistent>
            <v-card>
                <v-toolbar>
                    <v-toolbar-title>Change Slide Channel</v-toolbar-title>
                    <v-spacer></v-spacer>
                    <v-btn class="mx-2" color="green darken-1" @click="changeSlideChannelDialogShow = false">Done</v-btn>
                </v-toolbar>

                <v-list subheader>
                    <v-subheader>{{ Object.keys(slides).length }} Slide Channels</v-subheader>
                    <v-list-item
                        v-for="(channel, i, index) in slides"
                        track-by="$index"
                        :key="index"
                    >
                        <v-list-item-avatar size="64">
                            <v-img :src="channel[0]"></v-img>
                        </v-list-item-avatar>

                        <v-list-item-content>
                            <v-list-item-subtitle>Channel {{ i }}</v-list-item-subtitle>
                            <v-list-item-title>{{ slides[i][0] }}</v-list-item-title>
                        </v-list-item-content>

                        <v-list-item-icon>
                            <v-btn @click="changeSlideChannelDialogShow = false; slideChannel = i" color="green" class="mx-2" fab medium>
                                <v-icon>mdi-cursor-default-click</v-icon>
                            </v-btn>
                            <!-- <v-btn :disabled="index === 0" @click="rearrangeSlideChannel(i, 'up')" color="blue" class="mx-2" fab medium>
                                <v-icon>mdi-arrow-collapse-up</v-icon>
                            </v-btn>
                            <v-btn :disabled="index === Object.keys(slides).length - 1" @click="rearrangeSlideChannel(i, 'down')" color="blue" class="mx-2" fab medium>
                                <v-icon>mdi-arrow-collapse-down</v-icon>
                            </v-btn> -->
                            <v-btn :disabled="i === slideChannel || i === 'default'" @click="confirmDeleteSlideChannelDialogShow = true; confirmDeleteSlideChannelDialogWhich = i" color="red" class="mx-2" fab medium>
                                <v-icon>mdi-delete</v-icon>
                            </v-btn>
                        </v-list-item-icon>
                    </v-list-item>
                </v-list>

                <v-footer>
                    <v-text-field
                        placeholder="Create new channel here"
                        v-model="changeSlideChannelDialogText"
                        filled
                    >
                        <template slot="append-outer">
                            <v-btn class="mx-2" color="green darken-1" @click="addSlideChannel()">Add</v-btn>
                        </template>
                    </v-text-field>
                    <v-spacer></v-spacer>
                    <div>Current Slide Channel: <b>{{ slideChannel }}</b></div>
                </v-footer>
            </v-card>
        </v-dialog>
        
        <!-- Change Slide Channel Dialog -->
        
        <!-- Confirm Delete Slide Dialog -->
        
        <v-dialog v-model="confirmDeleteSlideDialogShow" persistent>
            <v-card>
                <v-toolbar>
                    <v-toolbar-title>Delete Slide</v-toolbar-title>
                    <v-spacer></v-spacer>
                    <v-btn class="mx-2" color="primary" @click="confirmDeleteSlideDialogShow = false">Close</v-btn>
                    <v-btn class="mx-2" color="red darken-1" @click="confirmDeleteSlideDialogShow = false; deleteSlide(confirmDeleteSlideDialogWhich)">Delete</v-btn>
                </v-toolbar>

                <v-card-title>Are you sure you want to delete slide {{ confirmDeleteSlideDialogWhich + 1 }}?</v-card-title>
                <v-card-subtitle>You cannot undo this action.</v-card-subtitle>
            </v-card>
        </v-dialog>
        
        <!-- Confirm Delete Slide Dialog -->
        
        <!-- Confirm Delete Slide Channel Dialog -->
        
        <v-dialog v-model="confirmDeleteSlideChannelDialogShow" persistent>
            <v-card>
                <v-toolbar>
                    <v-toolbar-title>Delete Slide Channel</v-toolbar-title>
                    <v-spacer></v-spacer>
                    <v-btn class="mx-2" color="primary" @click="confirmDeleteSlideChannelDialogShow = false">Close</v-btn>
                    <v-btn class="mx-2" color="red darken-1" @click="confirmDeleteSlideChannelDialogShow = false; deleteSlideChannel(confirmDeleteSlideChannelDialogWhich)">Delete</v-btn>
                </v-toolbar>

                <v-card-title>Are you sure you want to delete the slide channel {{ confirmDeleteSlideChannelDialogWhich }}?</v-card-title>
                <v-card-subtitle>You cannot undo this action.</v-card-subtitle>
            </v-card>
        </v-dialog>
        
        <!-- Confirm Delete Slide Channel Dialog -->
        
        <v-footer
            color="primary"
            app
        >
            <span class="">Current Slide Channel: <b>{{ slideChannel }}</b></span>
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
                // alert("SLIDES RECEIVED ON APP:" + JSON.stringify(receivedCommand.data));
                vue_this.initializeWebApp(receivedCommand.data);
            }
            
            if (receivedCommand.command === 'script-to-web-channel') {
                // alert("SLIDES RECEIVED ON APP:" + JSON.stringify(receivedCommand.data));
                vue_this.receiveChannelUpdate(receivedCommand.data);
            }
            
            if (receivedCommand.command === 'script-to-web-update-slide-state') {
                vue_this.updateSlideState(receivedCommand.data);
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
        slides: {
            'default': [
                './assets/logo.png'
            ]
            // 'Slide Deck 1': [
            //     'https://wallpapertag.com/wallpaper/full/d/5/e/154983-anime-girl-wallpaper-hd-1920x1200-for-hd.jpg',
            //     'https://wallpapertag.com/wallpaper/full/7/3/0/234884-anime-girls-wallpaper-3840x2160-ipad.jpg',
            //     'http://getwallpapers.com/wallpaper/full/2/7/b/596546.jpg',
            //     'https://images4.alphacoders.com/671/671041.jpg'
            // ],
            // 'Slide Deck 2': [
            //     'https://external-content.duckduckgo.com/iu/?u=https%3A%2F%2Fwallpapersite.com%2Fimages%2Fwallpapers%2Fquna-2560x1440-phantasy-star-online-2-4k-2336.jpg&f=1&nofb=1',
            //     'https://hdqwalls.com/wallpapers/anime-girl-aqua-blue-4k-gu.jpg',
            //     'https://images3.alphacoders.com/729/729085.jpg',
            //     'https://mangadex.org/images/groups/9766.jpg?1572281708'
            // ]
        },
        atp: {
            'use': null,
            'path': null
        },
        currentSlide: 0,
        presentationChannel: 'default-presentation-channel',
        slideChannel: 'default',
        // Add Slides Dialog
        addSlidesByURLDialogShow: false,
        addSlideByURLField: '',
        // Upload Slides Dialog
        uploadSlidesDialogShow: false,
        uploadSlidesDialogImgBBAPIKey: '3c004374cf70ad588aad5823ac2baaca', // Make this pull from UserData later.
        uploadSlidesDialogImgBBExpiry: 0, // Image expiry time: 0 = never. 43200 = 12 hours.
        uploadSlidesDialogFiles: null,
        uploadSlidesFailedSnackbar: false,
        uploadSlidesFailedError: '',
        uploadProcessingOverlay: false,
        uploadSlidesDialogRules: [
            // value => !value || value.size < 10000000 || 'Image size should be less than 10MB!'
        ],
        // Manage Slides Dialog
        manageSlidesDialogShow: false,
        // Change Presentation Channel Dialog
        changePresentationChannelDialogShow: false,
        changePresentationChannelDialogText: '',
        // Change Slide Channel Dialog
        changeSlideChannelDialogShow: false,
        changeSlideChannelDialogText: '',
        // Confirm Delete Slide Channel Dialog
        confirmDeleteSlideChannelDialogShow: false,
        confirmDeleteSlideChannelDialogWhich: '',
        // Confirm Delete Slide Dialog
        confirmDeleteSlideDialogShow: false,
        confirmDeleteSlideDialogWhich: ''
    }),
    watch: {
        currentSlide: function (newSlide, oldSlide) {
            if (newSlide !== oldSlide) {
                this.sendSlideChange(newSlide);
            }
        },
        slideChannel: function (newChannel, oldChannel) {
            if (newChannel !== oldChannel) {
                this.sendSlideChange(this.currentSlide)
            }
        },
        slides: {
            handler: function (newSlides) {
                this.sendSync(newSlides);
            },
            deep: true
        }
    },
    methods: {
        initializeWebApp: function (data) {
            // The data should already be parsed.
            // console.log("DATA RECEIVED ON INIT:" + JSON.stringify(data));
            var parsedUserData = data.userData; 
            
            // We are receiving the full slides, including slideChannels within.
            for (let i in parsedUserData.slides) {
                this.$set(this.slides, i, parsedUserData.slides[i]);
            }

            if (parsedUserData.presentationChannel) {
                this.presentationChannel = parsedUserData.presentationChannel;
            }

            if (parsedUserData.atp) {
                // console.log("setting userData for ATP: " + parsedUserData.atp.use + parsedUserData.atp.path);
                this.atp = parsedUserData.atp;
            }
        },
        deleteSlide: function (slideIndex) {
            this.slides[this.slideChannel].splice(slideIndex, 1);

            if (this.slides[this.slideChannel].length === 0) {
                this.manageSlidesDialogShow = false; // Hide the dialog if the user has deleted the last of the slides.
            }
        },
        deleteSlideChannel: function (slideChannelKey) {
            this.$delete(this.slides, slideChannelKey);
        },
        addSlideChannel: function () {
            this.$set(this.slides, this.changeSlideChannelDialogText, ['./assets/logo.png']);
        },
        addSlideByURL: function () {
            this.slides[this.slideChannel].push(this.addSlideByURLField);
            vue_this.currentSlide = vue_this.slides[vue_this.slideChannel].length - 1; // The array starts at 0, so the length will always be +1, so we account for that.
            this.addSlideByURLField = '';
        },
        uploadSlide: function () {
            // ImgBB Upload
            if (this.uploadSlidesDialogFiles) {
                for (var image of this.uploadSlidesDialogFiles) {
                    var urlToPost;
                    let imageFiles = new FormData();
                    imageFiles.append('image', image);
                    
                    if (this.uploadSlidesDialogImgBBExpiry !== 0) {
                        urlToPost = 'https://api.imgbb.com/1/upload?expiration=' 
                            + this.uploadSlidesDialogImgBBExpiry 
                            + '&key=' 
                            + this.uploadSlidesDialogImgBBAPIKey;
                    } else {
                        urlToPost = 'https://api.imgbb.com/1/upload?key='
                            + this.uploadSlidesDialogImgBBAPIKey;
                    }
                    
                    this.imgBBUpload(urlToPost, imageFiles)
                }
            }
        },
        imgBBUpload: function (urlToPost, imageFiles) {
            vue_this.uploadProcessingOverlay = true;
            window.$.ajax({
                type: 'POST',
                url: urlToPost,
                data: imageFiles,
                processData: false,
                contentType: false,
            })
                .done(function (result) {
                    vue_this.uploadSlidesDialogFiles = null; // Reset the file upload dialog field.
                    vue_this.uploadProcessingOverlay = false;
                    vue_this.slides[vue_this.slideChannel].push(result.data.display_url);
                    vue_this.currentSlide = vue_this.slides[vue_this.slideChannel].length - 1; // The array starts at 0, so the length will always be +1, so we account for that.
                    // console.info('success:', result);
                    return true;
                })
                .fail(function (result) {
                    vue_this.uploadSlidesDialogFiles = null; // Reset the file upload dialog field.
                    vue_this.uploadProcessingOverlay = false;
                    vue_this.uploadSlidesFailedSnackbar = true;
                    vue_this.uploadSlidesFailedError = result.responseJSON.error.message;
                    // console.info('fail:', result);
                    return true;
                })
        },
        rearrangeSlide: function (slideIndex, direction) {
            var newPosition;
            
            if (direction === "up") {
                newPosition = slideIndex - 1; // Up means lower in the array... up the list.
            } else if (direction === "down") {
                newPosition = slideIndex + 1; // Down means higher in the array... down the list.
            }
            
            var slideToMove = this.slides[this.slideChannel].splice(slideIndex, 1)[0];
            this.slides[this.slideChannel].splice(newPosition, 0, slideToMove);
        },
        // rearrangeSlideChannel: function (slideChannelIndex, direction) {
        //     var newPosition;
        // 
        //     if (direction === "up") {
        //         newPosition = slideChannelIndex - 1; // Up means lower in the array... up the list.
        //     } else if (direction === "down") {
        //         newPosition = slideChannelIndex + 1; // Down means higher in the array... down the list.
        //     }
        // 
        // 
        // },
        sendChannelUpdate: function () {
            this.presentationChannel = this.changePresentationChannelDialogText;
            this.changePresentationChannelDialogText = '';
        },
        receiveChannelUpdate: function (data) {
            this.presentationChannel = data;
        },
        updateSlideState: function (data) {
            // This function receives the message from sendSlideChange
            this.slideChannel = data.slideChannel;
            this.currentSlide = data.currentSlide;
        },
        sendSlideChange: function (slideIndex) {
            if (this.slides[this.slideChannel]) {
                this.sendAppMessage("web-to-script-slide-changed", {
                    'slide': this.slides[this.slideChannel][slideIndex],
                    'slideChannel': this.slideChannel,
                    'currentSlide': this.currentSlide
                });
            }
        },
        sendSync: function (slidesToSync) {
            if (!slidesToSync) {
                slidesToSync = this.slides;
            }
            
            this.sendAppMessage("web-to-script-sync-state", { 
                "slides": slidesToSync, 
                "presentationChannel": this.presentationChannel,
                "atp": this.atp
            });
        },
        sendAppMessage: function(command, data) {
            var JSONtoSend = {
                "app": "slider-client-web",
                "command": command,
                "data": data
            };
            
            if (!browserDevelopment()) {
                // eslint-disable-next-line
                EventBridge.emitWebEvent(JSON.stringify(JSONtoSend));
            } else {
                // alert(JSON.stringify(JSONtoSend));
            }
        }
    },
    created: function () {
        vue_this = this;
        
        this.sendAppMessage("ready", "");
    }
}
</script>