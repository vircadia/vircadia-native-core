<template>
  <v-app>
		<v-bottom-navigation v-model="bottomNav" id="navBar">
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

			<v-checkbox id="noSteamVR" class="mr-3 mt-7" v-model="noSteamVR" label="No SteamVR" value="true"></v-checkbox>
			
			
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
						<v-icon>unarchive</v-icon>
					</v-btn>
				</template>
				<span>Install</span>
			</v-tooltip>
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
						<v-progress-circular
							:size="25"
							:width="5"
							:rotate="90"
							:value="downloadProgress"
							color="red"
							v-if="showCloudDownload"
						>
						</v-progress-circular>
						<v-icon v-if="showCloudIcon">cloud_download</v-icon>
					</v-btn>
				</template>
				<span>Download</span>
			</v-tooltip>
			
			<v-btn
				v-on:click.native="launchInterface()"
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
	}
	console.info(downloadProgress);
	vue_this.downloadProgress = downloadProgress * 100;
});

import HelloWorld from './components/HelloWorld';
import FavoriteWorlds from './components/FavoriteWorlds';
import Settings from './components/Settings';

export default {
	name: 'App',
	components: {
		HelloWorld,
		FavoriteWorlds,
		Settings
	},
	methods: {
		launchInterface: function() {
			if(!this.noSteamVR) {
				this.noSteamVR = false;
			} 
			var allowMulti = false;
			if(this.$store.state.allowMultipleInterfaces) {
				allowMulti = true;
			}
			const { ipcRenderer } = require('electron');
			var exeLoc = ipcRenderer.sendSync('getAthenaLocation');
			ipcRenderer.send('launch-interface', { "exec": exeLoc, "steamVR": this.noSteamVR, "allowMultipleInterfaces": allowMulti});
		},
		launchBrowser: function(url) {
			const { shell } = require('electron')
			shell.openExternal(url);
		},
		downloadInterface: function() {
			if(!this.isDownloading) {
				const { ipcRenderer } = require('electron');
				ipcRenderer.send('downloadAthena');
			}
		},
		installInterface: function() {
			const { ipcRenderer } = require('electron');
			ipcRenderer.send('installAthena');
		}
	},
	created: function () {
		vue_this = this;
	},
	watch: {
		noSteamVR: function (newValue, oldValue) {
			this.$store.commit('setSteamVR', newValue);
		}
	},
	data: () => ({
		showTab: 'FavoriteWorlds',
		noSteamVR: false,
		downloadProgress: 0,
		isDownloading: false,
		showCloudIcon: true,
		showCloudDownload: false,
		disableInstallIcon: false,
	}),
};
</script>
