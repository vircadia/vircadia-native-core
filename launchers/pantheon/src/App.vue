<template>
  <v-app>
    <v-app-bar
      app
      color="#182b49"
      dark
			bottom="true"
    >
      <div class="d-flex align-center">
        <v-img
          alt="Project Athena Logo"
          class="shrink mr-2"
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
				
				<h2>Project Athena</h2>
				
				<v-btn
					href="https://github.com/kasenvr/project-athena"
					target="_blank"
					text
					left="true"
					class="ml-3"
				>
					<span class="mr-2">Github</span>
					<v-icon>mdi-open-in-new</v-icon>
				</v-btn>
      </div>

      <v-spacer></v-spacer>
			
			<v-btn
				href="https://github.com/kasenvr/project-athena"
				target="_blank"
				text
				right="true"
				class="mr-2"
			>
				<span class="mr-2">Launch</span>
				<v-icon>mdi-play</v-icon>
			</v-btn>

    </v-app-bar>
		
		<v-bottom-navigation
      v-model="bottomNav"
    >
      <v-btn value="recent">
        <span>Recent</span>
        <v-icon>mdi-history</v-icon>
      </v-btn>
  
      <v-btn value="favorites">
        <span>Favorites</span>
        <v-icon>mdi-heart</v-icon>
      </v-btn>
  
      <v-btn value="nearby">
        <span>Worlds</span>
        <v-icon>mdi-map-search-outline</v-icon>
      </v-btn>
    </v-bottom-navigation>
		
    <v-content>
      <HelloWorld/>
    </v-content>
		
  </v-app>
</template>

<script>
// import HelloWorld from './components/HelloWorld';

export default {
  name: 'App',

  components: {
    // HelloWorld,
  },

  data: () => ({
    //
  }),
};
</script>
