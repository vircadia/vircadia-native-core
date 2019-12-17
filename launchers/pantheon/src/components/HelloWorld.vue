<template>
  <v-container>
    <v-layout
      wrap
    >
		<v-card
	class="mx-auto"
	max-width="344"
>
	<v-img
		src="https://cdn.vuetifyjs.com/images/cards/sunshine.jpg"
		height="200px"
	></v-img>

	<v-card-title>
		The-Spot
	</v-card-title>

	<v-card-subtitle>
		1,000 miles of wonder
	</v-card-subtitle>

	<v-card-actions>
		<v-btn text>Share</v-btn>

		<v-btn
			color="purple"
			text
		>
			Explore
		</v-btn>

		<v-spacer></v-spacer>

		<v-btn
			icon
			@click="show = !show"
		>
			<v-icon>{{ show ? 'mdi-chevron-up' : 'mdi-chevron-down' }}</v-icon>
		</v-btn>
	</v-card-actions>

	<v-expand-transition>
		<div v-show="show">
			<v-divider></v-divider>

			<v-card-text>
				I'm a thing. But, like most politicians, he promised more than he could deliver. You won't have time for sleeping, soldier, not with all the bed making you'll be doing. Then we'll go with that data file! Hey, you add a one and two zeros to that or we walk! You're going to do his laundry? I've got to find a way to escape.
			</v-card-text>
		</div>
	</v-expand-transition>
</v-card>
    </v-layout>
  </v-container>
</template>

<script>

export default {
  name: 'HelloWorld',

  data: () => ({
    ecosystem: [
      {
        text: 'vuetify-loader',
        href: 'https://github.com/vuetifyjs/vuetify-loader',
      },
      {
        text: 'github',
        href: 'https://github.com/vuetifyjs/vuetify',
      },
      {
        text: 'awesome-vuetify',
        href: 'https://github.com/vuetifyjs/awesome-vuetify',
      },
    ],
    importantLinks: [
      {
        text: 'Documentation',
        href: 'https://vuetifyjs.com',
      },
      {
        text: 'Chat',
        href: 'https://community.vuetifyjs.com',
      },
      {
        text: 'Made with Vuetify',
        href: 'https://madewithvuejs.com/vuetify',
      },
      {
        text: 'Twitter',
        href: 'https://twitter.com/vuetifyjs',
      },
      {
        text: 'Articles',
        href: 'https://medium.com/vuetify',
      },
    ],
    whatsNext: [
      {
        text: 'Explore components',
        href: 'https://vuetifyjs.com/components/api-explorer',
      },
      {
        text: 'Select a layout',
        href: 'https://vuetifyjs.com/layout/pre-defined',
      },
      {
        text: 'Frequently Asked Questions',
        href: 'https://vuetifyjs.com/getting-started/frequently-asked-questions',
      },
    ],
  }),
};
</script>
