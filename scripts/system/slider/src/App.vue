<template>
    <v-app id="inspire">
        <v-navigation-drawer
            v-model="drawer"
            app
        >
            <v-list dense>
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
            </v-list>
        </v-navigation-drawer>

        <v-app-bar
            app
            color="primary"
            dark
        >
            <v-app-bar-nav-icon @click.stop="drawer = !drawer"></v-app-bar-nav-icon>
            <v-toolbar-title>Slides Presenter</v-toolbar-title>
            <v-spacer></v-spacer>
            <v-btn medium fab @click="currentSlide--">
                <v-icon>mdi-arrow-left</v-icon>
            </v-btn>
            <span class="mx-4">{{ currentSlide + 1 }} / {{ slides.length }}</span>
            <v-btn medium fab @click="currentSlide++">
                <v-icon>mdi-arrow-right</v-icon>
            </v-btn>
        </v-app-bar>
        
        <!-- Main Slider Control Area -->
        <v-main>
            <v-container
                class="fill-height"
                fluid
            >
                <v-carousel v-model="currentSlide" height="800">
                    <v-carousel-item
                        v-for="(slide, index) in slides"
                        track-by="$index"
                        :key="index"
                    >
                        <v-img
                            :src="slide"
                            height="100%"
                            class="grey darken-4"
                        ></v-img>
                    </v-carousel-item>
                </v-carousel>
            </v-container>
        </v-main>
        <!-- End Main Slider Control Area -->
        
        <!-- Add Slide by URL Dialog -->
        
        <v-dialog v-model="addSlidesByURLDialogShow" persistent>
            <v-card>
                <v-card-title class="headline">Add Slide by URL</v-card-title>
                <v-text-field
                    placeholder="Enter URL Here"
                    v-model="addSlideByURLField"
                    filled
                ></v-text-field>
                <v-card-actions>
                    <v-spacer></v-spacer>
                    <v-btn color="red darken-1" @click="addSlidesByURLDialogShow = false">Close</v-btn>
                    <v-btn color="green darken-1" @click="addSlidesByURLDialogShow = false; addSlideByURL()">Add</v-btn>
                </v-card-actions>
            </v-card>
        </v-dialog>
        
        <!-- Add Slide by URL Dialog -->
        
        <!-- Add Slide by Upload Dialog -->
        
        <v-dialog v-model="uploadSlidesDialogShow" persistent>
            <v-card>
                <v-card-title class="headline">Upload Slide</v-card-title>
                <v-file-input
                    v-model="uploadSlidesDialogFiles"
                    :rules="uploadSlidesDialogRules"
                    accept="image/png, image/jpeg"
                    placeholder="Pick an image to upload as a slide"
                    prepend-icon="mdi-image-search"
                    label="Slide"
                ></v-file-input>
                <v-card-actions>
                    <v-spacer></v-spacer>
                    <v-btn color="red darken-1" @click="uploadSlidesDialogShow = false">Close</v-btn>
                    <v-btn color="green darken-1" @click="uploadSlidesDialogShow = false; uploadSlide()">Upload</v-btn>
                </v-card-actions>
            </v-card>
        </v-dialog>
        
        <!-- Add Slide by Upload Dialog -->
        
        <!-- Manage Slides Dialog -->
        
        <v-dialog v-model="manageSlidesDialogShow" persistent>
            <v-card>
                <v-card-title class="headline">Manage Slides</v-card-title>
                <v-list subheader>
                    <v-subheader>Slides</v-subheader>

                    <v-list-item
                        v-for="(slide, i) in slides"
                        :key="slide"
                    >
                        <v-list-item-avatar>
                            <v-img :src="slide"></v-img>
                        </v-list-item-avatar>

                        <v-list-item-content>
                            <v-list-item-title v-text="slide"></v-list-item-title>
                        </v-list-item-content>

                        <v-list-item-icon>
                            <v-btn :disabled="i === 0" @click="rearrangeSlide(i, 'up')" color="blue" class="mx-2" fab medium>
                                <v-icon>mdi-arrow-collapse-up</v-icon>
                            </v-btn>
                            <v-btn :disabled="i === slides.length - 1" @click="rearrangeSlide(i, 'down')" color="blue" class="mx-2" fab medium>
                                <v-icon>mdi-arrow-collapse-down</v-icon>
                            </v-btn>
                            <v-btn @click="deleteSlide(i)" color="red" class="mx-2" fab medium>
                                <v-icon>mdi-delete</v-icon>
                            </v-btn>
                        </v-list-item-icon>
                    </v-list-item>
                </v-list>
                <v-card-actions>
                    <v-spacer></v-spacer>
                    <v-btn color="green" @click="manageSlidesDialogShow = false">Done</v-btn>
                </v-card-actions>
            </v-card>
        </v-dialog>
        
        <!-- Manage Slides Dialog -->
        
        <!-- Change Presentation Channel Dialog -->
        
        <v-dialog v-model="changePresentationChannelDialogShow" persistent>
            <v-card>
                <v-card-title class="headline">Change Presentation Channel</v-card-title>
                <v-text-field
                    placeholder="Enter channel here"
                    v-model="presentationChannel"
                    filled
                ></v-text-field>
                <v-card-actions>
                    <v-spacer></v-spacer>
                    <v-btn color="red darken-1" @click="changePresentationChannelDialogShow = false">Close</v-btn>
                    <v-btn color="green darken-1" @click="changePresentationChannelDialogShow = false; updatePresentationChannel()">Update</v-btn>
                </v-card-actions>
            </v-card>
        </v-dialog>
        
        <!-- Change Slide Channel Dialog -->
        
        <v-footer
            color="primary"
            app
        >
            <span class="white--text">Presenting on channel {{ presentationChannel }}</span>
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
        receivedCommand = JSON.parse(receivedCommand);
        // alert("RECEIVED COMMAND:" + receivedCommand.command)
        if (receivedCommand.app === "slider-app-control") {
        // We route the data based on the command given.
            if (receivedCommand.command === 'script-to-web-slides') {
                // alert("SLIDES RECEIVED ON APP:" + JSON.stringify(receivedCommand.data));
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
        slides: [
            'https://wallpapertag.com/wallpaper/full/d/5/e/154983-anime-girl-wallpaper-hd-1920x1200-for-hd.jpg',
            'https://wallpapertag.com/wallpaper/full/7/3/0/234884-anime-girls-wallpaper-3840x2160-ipad.jpg',
            'http://getwallpapers.com/wallpaper/full/2/7/b/596546.jpg'
        ],
        currentSlide: 0,
        presentationChannel: "ps-channel-1",
        // Add Slides Dialog
        addSlidesByURLDialogShow: false,
        addSlideByURLField: "",
        // Upload Slides Dialog
        uploadSlidesDialogShow: false,
        uploadSlidesDialogImgBBAPIKey: '3c004374cf70ad588aad5823ac2baaca', // Make this pull from UserData later.
        uploadSlidesdialogImgBBExpiry: 0, // 0 = never? 43200 = 12 hours
        uploadSlidesDialogFiles: null,
        uploadSlidesDialogRules: [
            value => !value || value.size < 10000000 || 'Image size should be less than 10MB!'
        ],
        // Manage Slides Dialog
        manageSlidesDialogShow: false,
        // Change Presentation Channel Dialog
        changePresentationChannelDialogShow: false,
    }),
    methods: {
        deleteSlide: function (slideIndex) {
            this.slides.splice(slideIndex, 1);
            
            if (this.slides.length === 0) {
                this.manageSlidesDialogShow = false; // Hide the dialog if the user has deleted the last of the slides.
            }
        },
        addSlideByURL: function () {
            this.slides.push(this.addSlideByURLField);
            this.addSlideByURLField = '';
        },
        uploadSlide: function () {
            // ImgBB Upload
            if (this.uploadSlidesDialogFiles) {
                let imageFiles = new FormData();
                imageFiles.append('image', this.uploadSlidesDialogFiles);
                
                window.$.ajax({
                    type: 'POST',
                    url: 'https://api.imgbb.com/1/upload?expiration=' + this.uploadSlidesdialogImgBBExpiry + '&key=' + this.uploadSlidesDialogImgBBAPIKey,
                    data: imageFiles,
                    processData: false,
                    contentType: false,
                })
                    .done(function (result) {
                        console.info('success:', result);
                    })
                    .fail(function (result) {
                        console.info('fail:', result);
                    })
            }
        },
        rearrangeSlide: function (slideIndex, direction) {
            var newPosition;
            
            if (direction === "up") {
                newPosition = slideIndex - 1; // Up means lower in the array... up the list.
            } else if (direction === "down") {
                newPosition = slideIndex + 1; // Down means higher in the array... down the list.
            }
            
            var slideToMove = this.slides.splice(slideIndex, 1)[0];
            this.slides.splice(newPosition, 0, slideToMove);
        },
        receiveSlides: function () {
        },
        updatePresentationChannel: function () {
            this.sendAppMessage("update-channel", this.presentationChannel);
        },
        sendAppMessage: function(command, data) {
            var JSONtoSend = {
                "app": "slide-presenter",
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
    }
}
</script>