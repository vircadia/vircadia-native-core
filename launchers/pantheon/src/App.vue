<template>
  <v-app>
		<v-bottom-navigation id="navBar">
			<v-btn disabled value="recent">
				<span>Recent</span>
				<v-icon>mdi-history</v-icon>
			</v-btn>
	
			<v-btn disabled v-on:click="toggleTab('Settings')" value="favorites">
				<span>Favorites</span>
				<v-icon>mdi-heart</v-icon>
			</v-btn>
	
			<v-btn disabled v-on:click="toggleTab('Settings')" value="nearby">
				<span>Worlds</span>
				<v-icon>mdi-map-search-outline</v-icon>
			</v-btn>
			
			<v-btn v-on:click="toggleTab('Settings')" value="settings">
				<span>Settings</span>
				<v-icon>mdi-settings-outline</v-icon>
			</v-btn>
		</v-bottom-navigation>
		
    <v-app-bar
      app
      color="#182b49"
      dark
			:bottom=true
			:fixed=true
			style="top: initial !important;"
    >
      <div class="d-flex align-center">
        <v-img
          alt="Project Athena Logo"
					id="titleIMG"
          class="shrink mr-2"
					v-on:click="launchBrowser('https://projectathena.io/')"
          contain
          src="./assets/logo_256_256.png"
          transition="scale-transition"
          width="40"
        />
				
				<h2 id="titleURL" v-on:click="launchBrowser('https://projectathena.io/')">Project Athena</h2>
				
				<v-btn
					v-on:click="launchBrowser('https://github.com/kasenvr/project-athena')"
					target="_blank"
					text
					:left=true
					class="ml-3"
				>
					<span class="mr-2">Github</span>
					<v-icon>mdi-open-in-new</v-icon>
				</v-btn>
      </div>

      <v-spacer></v-spacer>
				<v-btn
					v-on:click.native="downloadInterface()"
					:right=true
					color="blue"
					:tile=true
                    :disabled="isDownloading"
				>
                    <span style="font-size: 12px;">{{downloadText}}</span>
					<v-progress-circular
						:size="25"
						:width="5"
						:rotate="90"
						:value="downloadProgress"
						color="red"
						v-if="showCloudDownload"
                        class="ml-2"
					>
					</v-progress-circular>
					<v-icon class="ml-2" v-if="showCloudIcon">cloud_download</v-icon>
				</v-btn>
            
            <!-- <v-tooltip top>	
                <template v-slot:activator="{ on }">
                    <v-btn
                        v-on:click.native="installInterface()"
                        v-on="on"
                        :right=true
                        class=""
                        color="green"
                        :tile=true
                        :disabled="disableInstallIcon"
                    >
                        <span style="font-size: 12px;">Install<br/>Interface</span>
                        <v-icon class="ml-2">unarchive</v-icon>
                    </v-btn>
                </template>
                <span>Install</span>
            </v-tooltip> -->

            <div class="text-center">
            <v-menu top offset-y :close-on-content-click="false">
                <template v-slot:activator="{ on }">
                    <v-btn
                        color="primary"
                        dark
                        :tile=true
                        v-on="on"
                    >
                    Options
                    </v-btn>
                </template>
                <div style="background: rgba(255,255,255,0.8);">
                    <!-- <v-checkbox id="noSteamVR" class="" v-model="noSteamVR" label="No SteamVR" value="true"></v-checkbox> -->
                    <!-- <v-checkbox color="blue" id="multipleInterfaces" class="" v-model="allowMultipleInstances" @click="multipleInstances" label="Allow Multiple Instances" value="true"></v-checkbox> -->
                </div>
                <v-list
                    subheader
                    two-line
                    flat
                >
                    <v-subheader>Launch Options</v-subheader>

                    <v-list-item-group
                        multiple
                        v-model="launchOptions"
                    >
                        <v-list-item>
                            <template>
                                <v-list-item-action>
                                    <v-checkbox
                                        color="primary"
                                        :true-value="allowMultipleInstances"
                                        :input-value="allowMultipleInstances"
                                        v-model="allowMultipleInstances"
                                    ></v-checkbox>
                                </v-list-item-action>

                                <v-list-item-content @click="allowMultipleInstances = !allowMultipleInstances">
                                    <v-list-item-title>Simultaneous Interfaces</v-list-item-title>
                                    <v-list-item-subtitle>Allow multiple interfaces to be run simultaneously.</v-list-item-subtitle>
                                </v-list-item-content>
                            </template>
                        </v-list-item>
                        <v-list-item>
                            <template>
                                <v-list-item-action>
                                    <v-checkbox
                                        color="primary"
                                        :true-value="noSteamVR"
                                        :input-value="noSteamVR"
                                        v-model="noSteamVR"
                                    ></v-checkbox>
                                </v-list-item-action>

                                <v-list-item-content @click="noSteamVR = !noSteamVR">
                                    <v-list-item-title>Disable SteamVR</v-list-item-title>
                                    <v-list-item-subtitle>Disable launching and attaching SteamVR with interface.</v-list-item-subtitle>
                                </v-list-item-content>
                            </template>
                        </v-list-item>

                    </v-list-item-group>
                </v-list>
            </v-menu>
            </div>

            <v-btn
                v-on:click.native="attemptLaunchInterface()"
                :right=true
                class=""
                color="rgba(133, 0, 140, 0.8)"
                :tile=true
            >
                <span class="mr-2">Launch</span>
                <v-icon>mdi-play</v-icon>
            </v-btn>

    </v-app-bar>
		
    <v-content class="" id="mainContent">
			<!-- <iframe id="mainIframe" src="https://projectathena.io/" style="width: 100%; height: 100%;"></iframe>  -->
			<transition name="fade" mode="out-in">
				<component v-bind:is="showTab"></component>
			</transition>
    </v-content>
    
    <v-content class="">
        <transition name="fade" mode="out-in">
            <component @hideDialog="shouldShowDialog = false" v-if="shouldShowDialog" v-bind:is="showDialog"></component>
        </transition>
    </v-content>
		
  </v-app>
</template>

<script>
var vue_this;
const { ipcRenderer } = require('electron');

var downloaderDebounceReady = true;
function downloaderDebounce() {
    if(downloaderDebounceReady) {
        console.log("Ready.");
        downloaderDebounceReady = false;
        setTimeout(function() {
            downloaderDebounceReady = true;
        }, 5000); // ms before firing again.
        return true;
    } else {
        console.log("Not ready.");
        return false;
    }
}

ipcRenderer.on('download-installer-progress', (event, arg) => {
	var downloadProgress = arg.percent;
	if (downloadProgress < 1 && downloadProgress > 0) { // If downloading...
		vue_this.showCloudIcon = false;
		vue_this.showCloudDownload = true;
		vue_this.isDownloading = true;
        vue_this.disableInstallIcon = true;
        vue_this.downloadText = "Downloading";
        
        vue_this.downloadProgress = downloadProgress * 100;
	} else if (downloadProgress == 1) { // When done.
        if (downloaderDebounce()) {
            vue_this.installInterface();
            vue_this.showCloudIcon = true;
            vue_this.showCloudDownload = false;
            vue_this.disableInstallIcon = false;
            vue_this.isDownloading = false;
            vue_this.downloadText = "Download Interface";
            // vue_this.openDialog('DownloadComplete', true);
        }
	}
	console.info(downloadProgress);
});

ipcRenderer.on('state-loaded', (event, arg) => {
	console.info("STATE LOADED:", arg);
	
	if(arg.results.noSteamVR) {
		vue_this.$store.commit('mutate', {
			property: 'noSteamVR', 
			with: arg.results.noSteamVR
		});
		vue_this.noSteamVR = arg.results.noSteamVR;
	}
	if(arg.results.allowMultipleInstances) {
		vue_this.$store.commit('mutate', {
			property: 'allowMultipleInstances', 
			with: arg.results.allowMultipleInstances
		});
        vue_this.allowMultipleInstances = arg.results.allowMultipleInstances;
	}
	if(arg.results.selectedInterface) {
		vue_this.$store.commit('mutate', {
			property: 'selectedInterface', 
			with: arg.results.selectedInterface
		});
        ipcRenderer.send('set-current-interface', vue_this.$store.state.selectedInterface.folder);
	}
	if(arg.results.metaverseServer) {
		vue_this.$store.commit('mutate', {
			property: 'metaverseServer', 
			with: arg.results.metaverseServer
		});
		ipcRenderer.send('set-metaverse-server', arg.results.metaverseServer);
	}
    if(arg.results.currentLibraryFolder) {
        vue_this.$store.commit('mutate', {
			property: 'currentLibraryFolder', 
			with: arg.results.currentLibraryFolder
		});
	} else {
        ipcRenderer.send('set-library-folder-default');
    }
});

ipcRenderer.on('current-library-folder', (event, arg) => {
    vue_this.$store.commit('mutate', {
        property: 'currentLibraryFolder', 
        with: arg.libraryPath
    });
});

ipcRenderer.on('need-interface-selection', (event, arg) => {
    console.info("Need interface selection, nothing programmed yet.");
});

ipcRenderer.on('interface-list-for-launch', (event, arg) => {
    if(arg[0]) {
        var appName = Object.keys(arg[0])[0];
        var appLoc = arg[0][appName].location;
        var exeLoc = appLoc + "/interface.exe";
        vue_this.launchInterface(exeLoc);
        // console.info(arg[0]);
        // console.info(Object.keys(arg[0])[0]);
        // console.info(exeLoc);
    } else {
        vue_this.openDialog('NoInterfaceFound', true);
    }
});

ipcRenderer.on('no-installer-found', (event, arg) => {
    vue_this.openDialog('NoInstallerFound', true);
});

import HelloWorld from './components/HelloWorld';
import FavoriteWorlds from './components/FavoriteWorlds';
import Settings from './components/Settings';
// Dialogs
import DownloadComplete from './components/Dialogs/DownloadComplete'
import NoInstallerFound from './components/Dialogs/NoInstallerFound'
import NoInterfaceFound from './components/Dialogs/NoInterfaceFound'


export default {
	name: 'App',
	components: {
		HelloWorld,
		FavoriteWorlds,
		Settings,
        // Dialogs
        DownloadComplete,
        NoInstallerFound,
        NoInterfaceFound
	},
	methods: {
        toggleTab: function(tab) {
            if(this.showTab == tab) {
                this.showTab = "";
            } else {
                this.showTab = tab;
            }
        },
        openDialog: function(which, shouldShow) {
            // We want to reset the element first.
            this.showDialog = "";
            this.shouldShowDialog = false;
            // console.info(this.showDialog, this.shouldShowDialog);
            
            this.showDialog = which;
            this.shouldShowDialog = shouldShow;
            // console.info(this.showDialog, this.shouldShowDialog);
        },
		attemptLaunchInterface: function() {
			// var exeLoc = ipcRenderer.sendSync('get-athena-location'); // todo: check if that location exists first when using that, we need to default to using folder path + /interface.exe otherwise.
            var exeLoc;
            if (this.$store.state.selectedInterface) {
                exeLoc = this.$store.state.selectedInterface.folder + "interface.exe";
            }
            console.info("Attempting to launch interface, found exeLoc:",exeLoc);
            if(exeLoc) {
                this.launchInterface(exeLoc);
            } else {
                // this.selectInterfaceExe();
                // No, no more... we'll just default to selecting the first interface we find. You can select on your own time. UX baby.
                ipcRenderer.invoke('get-interface-list-for-launch');
            }
		},
        launchInterface: function(exeLoc) {
            var allowMulti = false;
            if(this.$store.state.allowMultipleInstances) {
                allowMulti = true;
            }
            ipcRenderer.send('launch-interface', { "exec": exeLoc, "steamVR": this.noSteamVR, "allowMultipleInstances": allowMulti});
        },
		launchBrowser: function(url) {
			const { shell } = require('electron');
			shell.openExternal(url);
		},
		downloadInterface: function() {
			if (!this.isDownloading) {
				const { ipcRenderer } = require('electron');
				ipcRenderer.send('download-athena');
			}
		},
		installInterface: function() {
			const { ipcRenderer } = require('electron');
			ipcRenderer.send('install-athena');
		},
        selectInterfaceExe: function() {
            const { ipcRenderer } = require('electron');
            ipcRenderer.send('set-athena-location');
        },
	},
	created: function () {
		const { ipcRenderer } = require('electron');
		vue_this = this;
		
		ipcRenderer.send('load-state');
        ipcRenderer.send('getLibraryFolder');
	},
	computed: {
		interfaceSelected () {
			return this.$store.state.selectedInterface;
		}
	},
	watch: {
		noSteamVR: function (newValue, oldValue) {
            if(newValue != oldValue) {
                this.$store.commit('mutate', {
                    property: 'noSteamVR', 
                    with: newValue
                });
            }
		},
        allowMultipleInstances: function (newValue, oldValue) {
            if(newValue != oldValue) {
                this.$store.commit('mutate', {
                    property: 'allowMultipleInstances', 
                    with: newValue
                });
            }
        },
		interfaceSelected (newVal, oldVal) {
			// console.log(`We have ${newVal} now!`);
		}
	},
	data: () => ({
        showTab: '',
        showDialog: '',
        shouldShowDialog: false,
		noSteamVR: false,
		allowMultipleInstances: false,
		downloadProgress: 0,
		isDownloading: false,
        downloadText: "Download Interface",
		showCloudIcon: true,
		showCloudDownload: false,
		disableInstallIcon: false,
        launchOptions: [],
	}),
};
</script>
