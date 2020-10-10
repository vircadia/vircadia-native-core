<template>
    <v-app>
        <v-main>
            <v-container
                class="fill-height"
                fluid
            >
                <v-card
                    height="100%"
                    width="100%"
                >
                    <v-toolbar
                        dark
                        flat
                    >
                        <v-toolbar-title>{{ triggeredEntity.name }}</v-toolbar-title>

                        <v-spacer></v-spacer>

                        <v-btn
                            small
                            fab
                            color="red"
                            @click="closeEntityMenu();"
                        >
                            <v-icon>mdi-close-thick</v-icon>
                        </v-btn>

                        <template v-slot:extension>
                            <v-tabs
                                v-model="tabs"
                                fixed-tabs
                            >
                                <v-tabs-slider></v-tabs-slider>
                                <v-tab
                                    class="primary--text"
                                >
                                    <v-icon>mdi-format-list-bulleted</v-icon>
                                </v-tab>
                                <v-tab
                                    class="primary--text"
                                >
                                    <v-icon>mdi-cube</v-icon>
                                </v-tab>
                            </v-tabs>
                        </template>
                    </v-toolbar>
                    
                    <v-tabs-items 
                        v-model="tabs"
                        height="100%"
                        width="100%"
                    >
                        <v-tab-item>
                            <!-- This needs to move to its own component. -->
                            <v-card
                                class="mx-auto"
                                height="100%"
                                width="100%"
                            >
                                <v-card-actions v-show="registeredEntityMenus[triggeredEntity.id]">
                                    <v-list>
                                        <v-subheader>ACTIONS</v-subheader>
                                        <v-list-item-group
                                            color="#385F73"
                                        >
                                            <v-list-item
                                                v-for="(item, i) in registeredEntityMenus[triggeredEntity.id]"
                                                :key="i"
                                                @click="triggeredMenuItem(item)"
                                                width="100%"
                                            >
                                                <v-list-item-content>
                                                    <v-list-item-title v-text="item"></v-list-item-title>
                                                </v-list-item-content>
                                            </v-list-item>
                                        </v-list-item-group>
                                    </v-list>
                                </v-card-actions>
                                <div v-show="!registeredEntityMenus[triggeredEntity.id]">
                                    <v-list-item-subtitle>No Actions Available</v-list-item-subtitle>
                                </div>
                            </v-card>
                        </v-tab-item>
                        <v-tab-item>
                            <!-- This needs to move to its own component. -->
                            <v-card
                                class="mx-auto"
                                height="100%"
                                width="100%"
                            >
                                <v-list-item two-line>
                                    <v-list-item-content>
                                        <div class="overline">
                                            Entity Type
                                        </div>
                                        <v-list-item-subtitle>{{ triggeredEntity.type }}</v-list-item-subtitle>
                                    </v-list-item-content>
                                </v-list-item>
                                <v-list-item two-line>
                                    <v-list-item-content>
                                        <div class="overline">
                                            Entity Host Type
                                        </div>
                                        <v-list-item-subtitle>{{ triggeredEntity.entityHostType }}</v-list-item-subtitle>
                                    </v-list-item-content>
                                </v-list-item>
                                <v-list-item two-line>
                                    <v-list-item-content>
                                        <div class="overline">
                                            Last Edited By
                                        </div>
                                        <v-list-item-subtitle>{{ triggeredEntity.lastEditedByName }}</v-list-item-subtitle>
                                    </v-list-item-content>
                                </v-list-item>
                                <v-list-item two-line>
                                    <v-list-item-content>
                                        <div class="overline">
                                            Description
                                        </div>
                                        <v-list-item-subtitle>{{ triggeredEntity.description }}</v-list-item-subtitle>
                                    </v-list-item-content>
                                </v-list-item>
                                <v-list-item two-line>
                                    <v-list-item-content>
                                        <div class="overline">
                                            Position (Vec3)
                                        </div>
                                        <v-list-item-subtitle>
                                            <span class="red--text">x: </span>{{ triggeredEntity.position.x }}<br/>
                                            <span class="green--text">y: </span>{{ triggeredEntity.position.y }}<br/>
                                            <span class="blue--text">z: </span>{{ triggeredEntity.position.z }}
                                        </v-list-item-subtitle>
                                    </v-list-item-content>
                                </v-list-item>
                                <v-list-item two-line>
                                    <v-list-item-content>
                                        <div class="overline">
                                            Rotation (Quat)
                                        </div>
                                        <v-list-item-subtitle>
                                            <span class="red--text">x: </span>{{ triggeredEntity.rotation.x }}<br/>
                                            <span class="green--text">y: </span>{{ triggeredEntity.rotation.y }}<br/>
                                            <span class="blue--text">z: </span>{{ triggeredEntity.rotation.z }}<br/>
                                            <span>w: </span>{{ triggeredEntity.rotation.w }}
                                        </v-list-item-subtitle>
                                    </v-list-item-content>
                                </v-list-item>
                            </v-card>
                        </v-tab-item>
                    </v-tabs-items>
                </v-card>
            </v-container>
        </v-main>
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

        // We route the data based on the command given.
        if (receivedCommand.command === 'script-to-web-registered-entity-menus') {
            vue_this.updateRegisteredEntityMenus(receivedCommand.data);
        }
        
        if (receivedCommand.command === 'script-to-web-triggered-entity-info') {
            vue_this.updateTriggeredEntityInfo(receivedCommand.data);
        }
    });
}

export default {
    name: 'App',
    components: {
    },
    data: () => ({
        tabs: 0,
        // Entity Context Menu
        registeredEntityMenus: {
            "{768542d0-e962-49e3-94fb-85651d56f5ae}": ['Item', 'Test']
        },
        triggeredEntity: {
            id: '{768542d0-e962-49e3-94fb-85651d56f5ae}',
            name: 'test',
            type: 'test',
            lastEditedBy: 'test',
            lastEditedByName: 'test',
            description: 'This is a test description.',
            entityHostType: 'Domain',
            position: {
                'x': 1,
                'y': 1,
                'z': 1
            },
            rotation: {
                'x': 1,
                'y': 1,
                'z': 1,
                'w': 1
            },
        }
    }),
    methods: {
        updateRegisteredEntityMenus: function (data) {
            // The data should already be parsed.
            // console.log("DATA RECEIVED ON INIT:" + JSON.stringify(data));
            this.registeredEntityMenus = data.registeredEntityMenus;
            this.triggeredEntity = data.lastTriggeredEntityInfo;
        },
        updateTriggeredEntityInfo: function (data) {
            // The data should already be parsed.
            // console.log("DATA RECEIVED ON TRIGGERED ENTITY:" + JSON.stringify(data));
            this.triggeredEntity = data;
        },
        triggeredMenuItem: function (menuItem) {
            var dataToSend = {
                'triggeredEntityID': this.triggeredEntity.id,
                'menuItem': menuItem
            }

            this.sendFrameworkMessage('menu-item-triggered', dataToSend);
        },
        closeEntityMenu: function() {
            this.sendFrameworkMessage('close-entity-menu', '');
        },
        sendFrameworkMessage: function(command, data) {
            var JSONtoSend = {
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

        this.$vuetify.theme.dark = 'true';
        this.sendFrameworkMessage('ready', '');
    }
}
</script>