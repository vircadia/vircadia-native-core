<template>
	<v-container id="favoriteWorldsContainer" style="">
		<template>
			<v-container fluid>
				<v-data-iterator
				:items="worlds"
				hide-default-footer
				>
					<!-- <template v-slot:header>
						<v-toolbar
							class="mb-2"
							color="cyan darken-5"
							dark
							flat
						>
						<v-toolbar-title>Favorite Worlds</v-toolbar-title>
						</v-toolbar>
					</template> -->

					<template v-slot:default="props">
						<v-row>
							<v-col
								v-for="item in props.items"
								:key="item.name"
								cols="12"
								sm="6"
								md="4"
								lg="3"
							>

							<v-card class="world-cards" width="230">
								<v-img
									:src="item.img"
									height="200px"
								></v-img>

								<v-card-title>
									{{ item.name }}
								</v-card-title>

								<!-- <v-card-subtitle>
									5 people
								</v-card-subtitle> -->

								<v-card-actions>

									<v-btn color="purple" text :href="item.url">
										Explore
									</v-btn>

									<!-- <v-spacer></v-spacer>

									<v-btn
										icon
										@click="show = !show"
									>
										<v-icon>{{ show ? 'mdi-chevron-up' : 'mdi-chevron-down' }}</v-icon>
									</v-btn> -->
								</v-card-actions>

								<!-- <v-expand-transition>
									<div v-show="show">
										<v-divider></v-divider>

										<v-card-text>
											I'm a thing. But, like most politicians, he promised more than he could deliver. You won't have time for sleeping, soldier, not with all the bed making you'll be doing. Then we'll go with that data file! Hey, you add a one and two zeros to that or we walk! You're going to do his laundry? I've got to find a way to escape.
										</v-card-text>
									</div>
								</v-expand-transition> -->
							</v-card>

							<v-divider></v-divider>

							</v-col>
						</v-row>
					</template>

				</v-data-iterator>
			</v-container>
		</template>
	</v-container>
</template>

<script>

export default {
  name: 'FavoriteWorlds',

  data: () => ({
    show: false,
		worlds: [
			// {
			// 	name: 'Athena Start',
			// 	url: 'serverless',
			// 	img: 'http://virtualrealitytimes.com/wp-content/uploads/2017/08/high-fidelity-og.jpg'
			// },
			{
				name: 'The-Spot',
				url: 'hifi://the-spot.projectathena.dev',
				img: 'https://images.pexels.com/photos/220201/pexels-photo-220201.jpeg?auto=compress&cs=tinysrgb&dpr=3&h=750&w=1260'
			},
			{
				name: 'Franny',
				url: 'hifi://franny.projectathena.dev',
				img: 'https://images.pexels.com/photos/220201/pexels-photo-220201.jpeg?auto=compress&cs=tinysrgb&dpr=3&h=750&w=1260'
			},
			{
				name: 'ThePalace',
				url: 'hifi://thepalace.projectathena.dev',
				img: 'https://images.pexels.com/photos/220201/pexels-photo-220201.jpeg?auto=compress&cs=tinysrgb&dpr=3&h=750&w=1260'
			},
			{
				name: 'Music',
				url: 'hifi://music.projectathena.dev',
				img: 'https://images.pexels.com/photos/220201/pexels-photo-220201.jpeg?auto=compress&cs=tinysrgb&dpr=3&h=750&w=1260'
			},
		]
  }),
};
</script>
