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
                <v-list-item link @click="changePresentationChannelDialogShow = !changePresentationChannelDialogShow">
                    <v-list-item-action>
                    <v-icon>mdi-remote</v-icon>
                    </v-list-item-action>
                    <v-list-item-content>
                        <v-list-item-title>Presentation Channel</v-list-item-title>
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
            <div v-show="slides.length > 0">
                <v-btn medium fab @click="currentSlide--">
                    <v-icon>mdi-arrow-left</v-icon>
                </v-btn>
                <span class="mx-4">{{ currentSlide + 1 }} / {{ slides.length }}</span>
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
                    <v-toolbar-title>Manage Slides</v-toolbar-title>
                    <v-spacer></v-spacer>
                    <v-btn class="mx-2" color="green" @click="manageSlidesDialogShow = false">Done</v-btn>
                </v-toolbar>
                <v-list subheader>
                    <v-subheader>Slides</v-subheader>

                    <v-list-item
                        v-for="(slide, i) in slides"
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
                            <v-btn @click="deleteSlide(i)" color="red" class="mx-2" fab medium>
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
            'http://getwallpapers.com/wallpaper/full/2/7/b/596546.jpg',
            'https://images4.alphacoders.com/671/671041.jpg',
            'https://get.wallhere.com/photo/anime-anime-girls-Dagashi-Kashi-Shidare-Hotaru-bikini-lingerie-clothing-undergarment-312172.jpg',
            'https://get.wallhere.com/photo/anime-manga-anime-girls-minimalism-simple-background-school-swimsuit-schoolgirl-blue-short-hair-1445637.jpg',
            'https://www.pixelstalk.net/wp-content/uploads/2016/06/Images-Download-Anime-Girl-Backgrounds.jpg',
            'https://mangadex.org/images/groups/9766.jpg?1572281708',
        ],
        currentSlide: 0,
        presentationChannel: "default-presentation-channel",
        // Add Slides Dialog
        addSlidesByURLDialogShow: false,
        addSlideByURLField: "",
        // Upload Slides Dialog
        uploadSlidesDialogShow: false,
        uploadSlidesDialogImgBBAPIKey: '3c004374cf70ad588aad5823ac2baaca', // Make this pull from UserData later.
        uploadSlidesDialogImgBBExpiry: 0, // Image expiry time: 0 = never. 43200 = 12 hours.
        uploadSlidesDialogFiles: null,
        uploadSlidesFailedSnackbar: false,
        uploadSlidesFailedError: '',
        uploadProcessingOverlay: false,
        uploadSlidesDialogRules: [
            value => !value || value.size < 10000000 || 'Image size should be less than 10MB!'
        ],
        // Manage Slides Dialog
        manageSlidesDialogShow: false,
        // Change Presentation Channel Dialog
        changePresentationChannelDialogShow: false,
        changePresentationChannelDialogText: '',
    }),
    watch: {
        currentSlide: function (newSlide) {
            this.sendSlideChange(newSlide);
        },
        slides: function (newSlides) {
            this.sendSync(newSlides);
        }
    },
    methods: {
        initializeWebApp: function (data) {
            // The data should already be parsed.
            // console.log("DATA RECEIVED ON INIT:" + JSON.stringify(data));
            var parsedUserData = data.userData; 
            
            if (parsedUserData.slides) {
                this.slides = parsedUserData.slides;
            }

            if (parsedUserData.presentationChannel) {
                this.presentationChannel = parsedUserData.presentationChannel;
            }
        },
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
                var urlToPost;
                
                imageFiles.append('image', this.uploadSlidesDialogFiles);
                
                if (this.uploadSlidesDialogImgBBExpiry !== 0) {
                    urlToPost = 'https://api.imgbb.com/1/upload?expiration=' 
                        + this.uploadSlidesDialogImgBBExpiry 
                        + '&key=' 
                        + this.uploadSlidesDialogImgBBAPIKey;
                } else {
                    urlToPost = 'https://api.imgbb.com/1/upload?key='
                        + this.uploadSlidesDialogImgBBAPIKey;
                }
                
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
                        vue_this.slides.push(result.data.display_url);
                        vue_this.currentSlide = vue_this.slides.length - 1; // The array starts at 0, so the length will always be +1, so we account for that.
                        // console.info('success:', result);
                    })
                    .fail(function (result) {
                        vue_this.uploadSlidesDialogFiles = null; // Reset the file upload dialog field.
                        vue_this.uploadProcessingOverlay = false;
                        vue_this.uploadSlidesFailedSnackbar = true;
                        vue_this.uploadSlidesFailedError = result.responseJSON.error.message;
                        // console.info('fail:', result);
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
        receiveSlides: function (data) {
            this.slides = data;
        },
        sendChannelUpdate: function () {
            this.presentationChannel = this.changePresentationChannelDialogText;
            this.changePresentationChannelDialogText = '';
            this.sendSync();
        },
        receiveChannelUpdate: function (data) {
            this.presentationChannel = data;
        },
        sendSlideChange: function (slideIndex) {
            console.log("Slide changed, web app." + slideIndex);
            this.sendAppMessage("web-to-script-slide-changed", this.slides[slideIndex]);
        },
        sendSync: function (slidesToSync) {
            if (!slidesToSync) {
                slidesToSync = this.slides;
            }
            
            this.sendAppMessage("web-to-script-sync-state", { 
                "slides": slidesToSync, 
                "presentationChannel": this.presentationChannel 
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