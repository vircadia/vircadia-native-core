<!--
//
//  App.vue
//
//  Created by Kalila L. on Feb 3 2021
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
-->

<template>
    <v-app>
        <v-main>
            <FirstRunWizard @send-message-to-script="sendMessageToScriptFromChild"></FirstRunWizard>
        </v-main>
    </v-app>
</template>

<script>
import FirstRunWizard from './components/FirstRunWizard';

// var vue_this;

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
        if (receivedCommand.command === 'command-goes-here') {
            // vue_this.method(receivedCommand.data);
        }
    });
}

export default {
    name: 'App',
    
    components: {
        FirstRunWizard
    },
    
    data: () => ({
        useDarkTheme: true,
        themeColors: {}
    }),
    
    methods: {
        sendMessageToScriptFromChild: function (commandAndData) {
            this.sendMessageToScript(commandAndData.command, commandAndData.data);
        },
        sendMessageToScript: function (command, data) {
            var JSONtoSend = {
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
        // vue_this = this;

        this.$vuetify.theme.dark = this.useDarkTheme;
        this.sendMessageToScript('ready', '');
    }
};
</script>
