<template>
		<v-container id="helloWorldContainer" style="">
		<v-layout wrap>
		<v-row no-gutters>
        <template v-for="n in 7">
          <v-col class="" :key="n">
						<v-card class="mx-0" width="290" style="margin-top: 10px;">
							<v-img
								src="http://virtualrealitytimes.com/wp-content/uploads/2017/08/high-fidelity-og.jpg"
								height="200px"
							></v-img>

							<v-card-title>
								The-Spot
							</v-card-title>

							<v-card-subtitle>
								5 people
							</v-card-subtitle>

							<v-card-actions>
								<v-btn text>Share</v-btn>

								<v-btn color="purple" text>
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
          </v-col>
          <v-responsive
            v-if="n === 3"
            :key="`width-${n}`"
            width="100%"
          ></v-responsive>
        </template>
      </v-row>
				
			</v-layout>
		</v-container>
</template>

<script>

export default {
  name: 'HelloWorld',

  data: () => ({
    show: false,
  }),
};
</script>
