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
                        flat
                    >
                        <v-toolbar-title 
                            v-show="triggeredEntity.name"
                        >
                            {{ triggeredEntity.name }}
                        </v-toolbar-title>
                        <v-toolbar-title 
                            v-show="!triggeredEntity.name"
                        >
                            Unnamed
                        </v-toolbar-title>

                        <v-spacer></v-spacer>

                        <v-btn
                            small
                            fab
                            color="primary"
                            @click="triggerSitOnEntity();"
                            class="mr-3"
                        >
                            <v-icon>mdi-sofa-single</v-icon>
                        </v-btn>
                        <v-btn
                            small
                            fab
                            color="primary"
                            @click="tabs = 2"
                            class="mr-3"
                        >
                            <v-icon>mdi-cog</v-icon>
                        </v-btn>
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
                                <v-tab
                                    class="primary--text"
                                    style="display: none;"
                                >
                                    <v-icon>mdi-cog</v-icon>
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
                                <v-list
                                    v-show="registeredEntityMenus[triggeredEntity.id]"
                                >
                                    <v-subheader>ACTIONS</v-subheader>
                                    <v-list>
                                        <div v-if="registeredEntityMenus[triggeredEntity.id] && registeredEntityMenus[triggeredEntity.id].buttons">
                                            <v-divider></v-divider>
                                            <v-list-item
                                                v-for="(item, i) in registeredEntityMenus[triggeredEntity.id].buttons"
                                                :key="i"
                                                @click="triggeredMenuItem(item.name)"
                                                width="100%"
                                            >
                                                <v-list-item-content>
                                                    <v-list-item-title v-text="item.name"></v-list-item-title>
                                                </v-list-item-content>
                                            </v-list-item>
                                        </div>
                                        <div v-if="registeredEntityMenus[triggeredEntity.id] && registeredEntityMenus[triggeredEntity.id].colorPickers">
                                            <v-divider></v-divider>
                                            <div
                                                v-for="(item, i) in registeredEntityMenus[triggeredEntity.id].colorPickers"
                                                :key="i"
                                                width="100%"
                                            >
                                                <v-list-item-content>
                                                    <v-color-picker
                                                        @update:color="colorPickerUpdated(item.name, $event)"
                                                        :value="item.initialColor"
                                                    ></v-color-picker>
                                                </v-list-item-content>
                                            </div>
                                        </div>
                                        <div v-if="registeredEntityMenus[triggeredEntity.id] && registeredEntityMenus[triggeredEntity.id].sliders">
                                            <v-divider></v-divider>
                                            <div
                                                v-for="(item, i) in registeredEntityMenus[triggeredEntity.id].sliders"
                                                :key="i"
                                                width="100%"
                                            >
                                                <v-list-item-content>
                                                    <v-slider
                                                        v-on:change="sliderUpdated(item.name, $event)"
                                                        :label="item.name"
                                                        :color="item.color"
                                                        :value="item.initialValue"
                                                        :min="item.minValue"
                                                        :max="item.maxValue"
                                                        :step="item.step"
                                                    ></v-slider>
                                                </v-list-item-content>
                                            </div>
                                        </div>
                                        <div v-if="registeredEntityMenus[triggeredEntity.id] && registeredEntityMenus[triggeredEntity.id].textFields">
                                            <v-divider></v-divider>
                                            <div
                                                v-for="(item, i) in registeredEntityMenus[triggeredEntity.id].textFields"
                                                :key="i"
                                                width="100%"
                                            >
                                                <v-list-item-content>
                                                    <v-text-field
                                                        :label="item.name"
                                                        :value="item.initialValue"
                                                        :hint="item.hint"
                                                        v-model="textFields[item.name]"
                                                    >
                                                        <v-tooltip slot="append" left>
                                                            <template v-slot:activator="{ on, attrs }">
                                                                <v-icon
                                                                    @click="textFieldUpdated(item.name)" 
                                                                    v-bind="attrs"
                                                                    v-on="on"
                                                                    color="green"
                                                                >
                                                                    mdi-check
                                                                </v-icon>
                                                            </template>
                                                            <span>Apply</span>
                                                        </v-tooltip>
                                                    </v-text-field>
                                                </v-list-item-content>
                                            </div>
                                        </div>
                                    </v-list>
                                </v-list>
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
                        <v-tab-item>
                            <!-- This needs to move to its own component. -->
                            <v-card
                                class="mx-auto"
                                height="100%"
                                width="100%"
                            >
                                <v-card-text>
                                    <v-card-title>
                                        Trigger Entity Menu
                                    </v-card-title>
                                    <v-divider></v-divider>
                                    <v-row>
                                        <v-card-title>
                                            <v-checkbox
                                                v-model="settings.entityMenu.useMouseTriggers"
                                                hide-details
                                                class="shrink mr-2 mt-0"
                                            ></v-checkbox>
                                            Mouse / Controller
                                        </v-card-title>
                                    </v-row>
                                    <v-row>
                                        <v-combobox
                                            v-model="settings.entityMenu.selectedMouseTriggers"
                                            :items="settings.entityMenu.possibleMouseTriggers"
                                            :disabled="!settings.entityMenu.useMouseTriggers"
                                            label="Triggered by"
                                            multiple
                                            outlined
                                            dense
                                        ></v-combobox>
                                    </v-row>
                                    <v-row>
                                        <v-combobox
                                            v-model="settings.entityMenu.selectedMouseModifiers"
                                            :items="settings.entityMenu.possibleMouseModifiers"
                                            :disabled="true || !settings.entityMenu.useMouseTriggers"
                                            label="Modifiers"
                                            multiple
                                            outlined
                                            dense
                                        ></v-combobox>
                                    </v-row>
                                </v-card-text>
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
        
        if (receivedCommand.command === 'script-to-web-update-settings') {
            if (receivedCommand.data.settings === null || receivedCommand.data.settings === '') {
                return;
            }
            console.log('receivedCommand.data: ' + JSON.stringify(receivedCommand.data));
            vue_this.updateSettings(receivedCommand.data);
        }
    });
}

export default {
    name: 'App',
    components: {
    },
    data: () => ({
        // Settings
        settings: {
            entityMenu: {
                useMouseTriggers: true,
                selectedMouseTriggers: [],
                possibleMouseTriggers: [
                    'Primary',
                    'Secondary',
                    'Tertiary'
                ],
                possibleMouseModifiers: [
                    'Shift',
                    'Meta',
                    'Control',
                    'Alt'
                ]
            }
        },
        // Dynamic Menu Item Data
        textFields: {},
        // General
        tabs: 0,
        // Entity Context Menu
        registeredEntityMenus: {
            "{768542d0-e962-49e3-94fb-85651d56f5ae}": {
                buttons: [
                    {
                        name: 'Equip'
                    }
                ]
            }
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
    computed: {
    }, 
    methods: {
        updateRegisteredEntityMenus: function (data) {
            // The data should already be parsed.
            // console.log("DATA RECEIVED ON UPDATE REGISTERED ENTITY MENU ITEMS:" + JSON.stringify(data));
            this.registeredEntityMenus = data.registeredEntityMenus;
            this.triggeredEntity = data.lastTriggeredEntityInfo;
        },
        updateTriggeredEntityInfo: function (data) {
            // The data should already be parsed.
            // console.log("DATA RECEIVED ON TRIGGERED ENTITY:" + JSON.stringify(data));
            this.triggeredEntity = data;
        },
        updateSettings: function (data) {
            this.settings = data.settings;
        },
        triggeredMenuItem: function (menuItem) {
            var dataToSend = {
                'triggeredEntityID': this.triggeredEntity.id,
                'menuItem': menuItem
            }

            this.sendFrameworkMessage('menu-item-triggered', dataToSend);
        },
        triggerSitOnEntity: function () {
            var dataToSend = {
                'triggeredEntityID': this.triggeredEntity.id,
                'sit': true
            }

            this.sendFrameworkMessage('sit-on-entity-triggered', dataToSend);
        },
        colorPickerUpdated: function (colorPicker, colorEvent) {
            var dataToSend = {
                'triggeredEntityID': this.triggeredEntity.id,
                data: {
                    'name': colorPicker,
                    'colors': colorEvent
                }
            }

            this.sendFrameworkMessage('dynamic-menu-item-triggered', dataToSend);
        },
        sliderUpdated: function (slider, sliderEvent) {
            var dataToSend = {
                'triggeredEntityID': this.triggeredEntity.id,
                data: {
                    'name': slider,
                    'value': sliderEvent
                }
            }

            this.sendFrameworkMessage('dynamic-menu-item-triggered', dataToSend);
        },
        textFieldUpdated: function (textField) {
            var dataToSend = {
                'triggeredEntityID': this.triggeredEntity.id,
                data: {
                    'name': textField,
                    'value': this.textFields[textField]
                }
            }

            this.sendFrameworkMessage('dynamic-menu-item-triggered', dataToSend);
        },
        closeEntityMenu: function () {
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
    watch: {
        settings: {
            deep: true,
            handler() {
                var dataToSend = {
                    settings: this.settings
                }

                this.sendFrameworkMessage('web-to-script-settings-changed', dataToSend);
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