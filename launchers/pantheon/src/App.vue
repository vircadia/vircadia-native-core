<template>
  <v-app>
    <v-app-bar
      app
      color="#182b49"
      dark
			:bottom=true
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

        <!-- <v-img
          alt="Project Athena"
          class="shrink mt-1 hidden-sm-and-down"
          contain
          min-width="100"
          src="https://cdn.vuetifyjs.com/images/logos/vuetify-name-dark.png"
          width="100"
        /> -->
				
				<h2 id="titleURL" v-on:click="launchBrowser('https://projectathena.io/')">Project Athena</h2>
				
				<v-btn
					href="https://github.com/kasenvr/project-athena"
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
			
			<v-btn
				v-on:click.native="launchInterface()"
				text
				:right=true
				class="mr-2"
			>
				<span class="mr-2">Launch</span>
				<v-icon>mdi-play</v-icon>
			</v-btn>

    </v-app-bar>
		
		<v-bottom-navigation v-model="bottomNav" id="navBar">
      <v-btn disabled value="recent">
        <span>Recent</span>
        <v-icon>mdi-history</v-icon>
      </v-btn>
  
      <v-btn disabled v-on:click="showTab = 'FavoriteWorlds'" value="favorites">
        <span>Favorites</span>
        <v-icon>mdi-heart</v-icon>
      </v-btn>
  
      <v-btn disabled v-on:click="showTab = 'HelloWorld'" value="nearby">
        <span>Worlds</span>
        <v-icon>mdi-map-search-outline</v-icon>
      </v-btn>
    </v-bottom-navigation>
		
    <v-content class="" id="mainContent">
			<!-- <iframe id="mainIframe" src="https://projectathena.io/" style="width: 100%; height: 100%;"></iframe>  -->
			<transition name="fade" mode="out-in">
				<component v-bind:is="showTab"></component>
			</transition>
    </v-content>
		
  </v-app>
</template>

<script>
import HelloWorld from './components/HelloWorld';
import FavoriteWorlds from './components/FavoriteWorlds';

export default {
  name: 'App',

  components: {
		HelloWorld,
		FavoriteWorlds
  },
	methods: {
		launchInterface: function() {
			if(!this.noSteamVR) {
				this.noSteamVR = false;
			} 
			
			const { ipcRenderer } = require('electron');
			var exeLoc = ipcRenderer.sendSync('getAthenaLocation');
			ipcRenderer.send('launch-interface', { "exec": exeLoc, "steamVR": this.noSteamVR});
		},
		launchBrowser: function(url) {
			const { shell } = require('electron')
			shell.openExternal(url);
		},
	},
  data: () => ({
		showTab: 'NOTHING',
		// showTab: 'HelloWorld',
		noSteamVR: false,
  }),
};
</script>
