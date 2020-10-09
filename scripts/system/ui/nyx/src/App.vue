<template>
    <v-app>
        <v-main>
            <v-container
                class="fill-height"
                fluid
            >
                <!-- This needs to move to its own component. -->
                <v-card
                    class="mx-auto"
                    height="100%"
                    width="100%"
                >
                    <v-list-item three-line>
                        <v-list-item-content>
                            <div class="overline mb-4">
                                {{ clickedEntity.name }}
                            </div>
                            <!-- <v-list-item-title class="headline mb-1">
                                {{ clickedEntity.name }}
                            </v-list-item-title> -->
                            <v-list-item-subtitle>Last Edited By:</v-list-item-subtitle>
                            <v-list-item-subtitle>{{ clickedEntity.lastEditedByName }}</v-list-item-subtitle>
                        </v-list-item-content>
                    </v-list-item>

                    <v-card-actions v-show="registeredEntityMenus[clickedEntity.id]">
                        <v-list dense>
                            <v-list-item-group
                                v-model="item"
                                color="primary"
                            >
                                <v-list-item
                                    v-for="(item, i) in registeredEntityMenus[clickedEntity.id]"
                                    :key="i"
                                >
                                    <v-list-item-content>
                                        <v-list-item-title v-text="item"></v-list-item-title>
                                    </v-list-item-content>
                                </v-list-item>
                            </v-list-item-group>
                        </v-list>
                    </v-card-actions>
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
        
        if (receivedCommand.command === 'script-to-web-clicked-entity-info') {
            vue_this.updateClickedEntityInfo(receivedCommand.data);
        }
    });
}

export default {
    name: 'App',
    components: {
    },
    data: () => ({
        registeredEntityMenus: {},
        clickedEntity: {
            id: 'test',
            name: 'test',
            lastEditedBy: 'test',
            lastEditedByName: 'test'
        }
    }),
    methods: {
        updateRegisteredEntityMenus: function (data) {
            // The data should already be parsed.
            console.log("DATA RECEIVED ON INIT:" + JSON.stringify(data));
            this.registeredEntityMenus = data.registeredEntityMenus;
        },
        updateClickedEntityInfo: function (data) {
            // The data should already be parsed.
            console.log("DATA RECEIVED ON CLICKED ENTITY:" + JSON.stringify(data));
            this.clickedEntity = data;
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