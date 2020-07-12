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
                <span class="mx-4">{{ currentSlide }} / {{slides.length - 1}}</span>
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
                        v-for="(slide) in slides"
                        :key="slide"
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
                    :rules="uploadSlidesDialogRules"
                    accept="image/png, image/jpeg"
                    placeholder="Pick an avatar"
                    prepend-icon="mdi-image-search"
                    label="Slide"
                ></v-file-input>
                <v-card-actions>
                    <v-spacer></v-spacer>
                    <v-btn color="red darken-1" @click="addSlidesByURLDialogShow = false">Close</v-btn>
                    <v-btn color="green darken-1" @click="addSlidesByURLDialogShow = false; addSlideByURL()">Add</v-btn>
                </v-card-actions>
            </v-card>
        </v-dialog>
        
        <!-- Add Slide by Upload Dialog -->
        
        <v-footer
            color="primary"
            app
        >
            <span class="white--text">&copy; {{ new Date().getFullYear() }}</span>
        </v-footer>
    </v-app>
</template>

<script>

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
        // Add Slides Dialog
        addSlidesByURLDialogShow: false,
        addSlideByURLField: "",
        // Upload Slides Dialog
        uploadSlidesDialogShow: false,
        uploadSlidesDialogRules: [
            value => !value || value.size < 10000000 || 'Image size should be less than 10MB!'
        ],
        // Manage Slides Dialog
        manageSlidesDialogShow: false,
    }),
    methods: {
        deleteSlide: function () {
        },
        addSlideByURL: function () {
            this.slides.push(this.addSlideByURLField);
            this.addSlideByURLField = '';
        },
        uploadSlide: function () {
        },
        rearrangeSlide: function () {
        },
        receiveSlides: function (slides) {
        }  
    }
}
</script>