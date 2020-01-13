<template>
  <v-app>
		<v-bottom-navigation id="navBar">
			<v-btn disabled value="recent">
				<span>Recent</span>
				<v-icon>mdi-history</v-icon>
			</v-btn>
	
			<v-btn v-on:click="showTab = 'FavoriteWorlds'" value="favorites">
				<span>Favorites</span>
				<v-icon>mdi-heart</v-icon>
			</v-btn>
	
			<v-btn disabled v-on:click="showTab = 'HelloWorld'" value="nearby">
				<span>Worlds</span>
				<v-icon>mdi-map-search-outline</v-icon>
			</v-btn>
			
			<v-btn v-on:click="showTab = 'Settings'" value="settings">
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
			
			<v-tooltip top>
				<template v-slot:activator="{ on }">
					<v-btn
						v-on:click.native="downloadInterface()"
						v-on="on"
						:right=true
						class=""
						color="blue"
						:tile=true
					>
                        <span style="font-size: 12px;">Download<br/>Interface</span>
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
				</template>
				<span>Download</span>
			</v-tooltip>
            
            <v-tooltip top>	
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
            </v-tooltip>

            <v-checkbox id="noSteamVR" class="ml-5 mr-3 mt-7" v-model="noSteamVR" label="No SteamVR" value="true"></v-checkbox>
			
            <v-tooltip top>
                <template v-slot:activator="{ on }">
                    <v-btn
                        v-on:click.native="launchInterface()"
                        v-on="on"
                        :right=true
                        class=""
                        color="rgba(133, 0, 140, 0.8)"
                        :tile=true
                    >
                        <span class="mr-2">Launch</span>
                        <v-icon>mdi-play</v-icon>
                    </v-btn>
                </template>
                <span>Launch</span>
            </v-tooltip>

    </v-app-bar>
		
    <v-content class="" id="mainContent">
			<!-- <iframe id="mainIframe" src="https://projectathena.io/" style="width: 100%; height: 100%;"></iframe>  -->
			<transition name="fade" mode="out-in">
				<component v-bind:is="showTab"></component>
			</transition>
    </v-content>
    
    <v-dialog
        width="500"
        v-model="showDownloadDone"
    >
        <v-card>
            <v-card-title
                class="headline"
                primary-title
                dark
            >
                Notice
            </v-card-title>
    
            <v-card-text>
                The latest version of Interface is done downloading! You can install it now or press the install button in the bar below later.
            </v-card-text>
    
            <v-divider></v-divider>
    
            <v-card-actions>
                <v-spacer></v-spacer>
                <v-btn
                    color="primary"
                    text
                    @click="installInterface()"
                >
                    Install
                </v-btn>
                <v-btn
                    color="primary"
                    text
                    @click="showDownloadDone = false"
                >
                    Dismiss
                </v-btn>
            </v-card-actions>
        </v-card>
    </v-dialog>
    
    <v-content class="">
        <transition name="fade" mode="out-in">
            <component v-bind:is="showDialog"></component>
        </transition>
    </v-content>
		
  </v-app>
</template>

<script>
var vue_this;
const { ipcRenderer } = require('electron');
ipcRenderer.on('download-installer-progress', (event, arg) => {
	var downloadProgress = arg.percent;
	if(downloadProgress < 1) { // If downloading...
		vue_this.showCloudIcon = false;
		vue_this.showCloudDownload = true;
		vue_this.isDownloading = true;
	} else { // When done.
		vue_this.showCloudIcon = true;
		vue_this.showCloudDownload = false;
		vue_this.disableInstallIcon = false;
		vue_this.isDownloading = false;
        vue_this.showDownloadDone = true;
	}
	console.info(downloadProgress);
	vue_this.downloadProgress = downloadProgress * 100;
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
	}
	if(arg.results.selectedInterface) {
		vue_this.$store.commit('mutate', {
			property: 'selectedInterface', 
			with: arg.results.selectedInterface
		});
	}
	if(arg.results.metaverseServer) {
		vue_this.$store.commit('mutate', {
			property: 'metaverseServer', 
			with: arg.results.metaverseServer
		});
		ipcRenderer.send('set-metaverse-server', arg.results.metaverseServer);
	}
});

ipcRenderer.on('current-library-folder', (event, arg) => {
    vue_this.$store.commit('mutate', {
        property: 'currentLibraryFolder', 
        with: arg.libraryPath
    });
});

import HelloWorld from './components/HelloWorld';
import FavoriteWorlds from './components/FavoriteWorlds';
import Settings from './components/Settings';

import RequireLibrary from './components/Dialogs/RequireLibrary'

export default {
	name: 'App',
	components: {
		HelloWorld,
		FavoriteWorlds,
		Settings,
        RequireLibrary
	},
	methods: {
		launchInterface: function() {
			if(!this.noSteamVR) {
				this.noSteamVR = false;
			} 
			var allowMulti = false;
			if(this.$store.state.allowMultipleInstances) {
				allowMulti = true;
			}
			const { ipcRenderer } = require('electron');
			// var exeLoc = ipcRenderer.sendSync('get-athena-location'); // todo: check if that location exists first when using that, we need to default to using folder path + /interface.exe otherwise.
            if(this.$store.state.selectedInterface.folder) {
                var exeLoc = this.$store.state.selectedInterface.folder + "interface.exe";
            }
            console.info("exeLoc:",exeLoc);
            if(exeLoc) {
                ipcRenderer.send('launch-interface', { "exec": exeLoc, "steamVR": this.noSteamVR, "allowMultipleInstances": allowMulti});
            } else {
                this.selectInterfaceExe();
            }
		},
		launchBrowser: function(url) {
			const { shell } = require('electron')
			shell.openExternal(url);
		},
		downloadInterface: function() {
			if(!this.isDownloading) {
				const { ipcRenderer } = require('electron');
				ipcRenderer.send('download-athena');
			}
		},
		installInterface: function() {
			const { ipcRenderer } = require('electron');
			ipcRenderer.send('installAthena');
		},
        selectInterfaceExe: function() {
            const { ipcRenderer } = require('electron');
            ipcRenderer.send('set-athena-location');
        }
	},
	created: function () {
		const { ipcRenderer } = require('electron');
		vue_this = this;
		
		ipcRenderer.send('load-state');
		
		// Load saved selected interface.	
		if (this.$store.selectedInterface) {
			ipcRenderer.send('setCurrentInterface', this.$store.state.selectedInterface.folder);
			this.$store.commit('mutate', {
				property: 'interfaceSelectionRequired', 
				with: false
			});
		}
        
        if(this.$store.state.selectedInterface) {
            this.$store.commit('mutate', {
                property: 'interfaceSelectionRequired', 
                with: false
            });
            const { ipcRenderer } = require('electron');			
            ipcRenderer.send('setCurrentInterface', this.$store.state.selectedInterface.folder);
        }
        
        ipcRenderer.send('getLibraryFolder');
	},
	computed: {
		interfaceSelected () {
			return this.$store.state.selectedInterface;
		}
	},
	watch: {
		noSteamVR: function (newValue, oldValue) {
			this.$store.commit('mutate', {
				property: 'noSteamVR', 
				with: newValue
			});
		},
		interfaceSelected (newVal, oldVal) {
			// console.log(`We have ${newVal} now!`);
		}
	},
	data: () => ({
		showTab: 'FavoriteWorlds',
        showDialog: 'RequireLibrary',
		noSteamVR: false,
		allowMultipleInstances: false,
		downloadProgress: 0,
		isDownloading: false,
		showCloudIcon: true,
		showCloudDownload: false,
		disableInstallIcon: false,
        showDownloadDone: false,
	}),
};
</script>
