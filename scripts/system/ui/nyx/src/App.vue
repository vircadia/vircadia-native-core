<template>
    <div id="app">
        <DropdownMenu
            v-bind:menuItems="menuItems"
        ></DropdownMenu>
    </div>
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
        // We route the data based on the command given.
        if (receivedCommand.command === 'script-to-web-initialize') {
            // alert("SLIDES RECEIVED ON APP:" + JSON.stringify(receivedCommand.data));
            vue_this.initializeWebFramework(receivedCommand.data);
        }
    });
}


import DropdownMenu from './components/DropdownMenu.vue'

export default {
    name: 'App',
    components: {
        DropdownMenu
    },
    data: () => ({
        menuItems: [
            'Test',
            'Waifu',
            'Lol'
        ]
    }),
    methods: {
        initializeWebFramework: function (data) {
            // The data should already be parsed.
            console.log("DATA RECEIVED ON INIT:" + JSON.stringify(data));
            this.menuItems = data.menuItems;
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

        this.sendFrameworkMessage("ready", "");
    }
}
</script>