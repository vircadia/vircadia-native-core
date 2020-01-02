<template>
	<v-app id="settingsContainer">
		<v-content>
			<v-container
				class="pt-8"
				fluid
			>
				<v-row
					align="center"
					justify="center"
				>
					<v-card class="elevation-12" style="width: 80%;">
						<v-toolbar
							color="primary"
							dark
							flat
						>
							<v-toolbar-title>Settings</v-toolbar-title>
							<v-spacer />
							
							<v-menu
								transition="slide-y-transition"
								bottom
							>
								<template v-slot:activator="{ on }">
									<v-btn
										v-on="on"
									>
									Set Client & Profile
									</v-btn>
								</template>
								<v-list>
									<v-list-item
										v-for="(item, i) in interfaceFolders"
										:key="i"
									>
									<v-list-item-title>{{ item.folder }}</v-list-item-title>
									</v-list-item>
								</v-list>
							</v-menu>
							
						</v-toolbar>
						
						<h2 class="ml-3 mt-3">General</h2>
						
						<v-layout row pr-5 pt-5 pl-10>
							<v-flex md6>
								<v-btn 
									v-on:click.native="selectInterface()"
									:right=true
									class=""
									:tile=true
								>
									<span class="mr-2">Select Interface executable</span>
									<v-icon>settings_applications</v-icon>
								</v-btn>
							</v-flex>
						</v-layout>
						
						<v-layout row pr-5 pt-5 pl-10>
							<v-flex md6>
								<v-btn
									v-on:click.native="setLibrary()"
									:right=true
									class=""
									:tile=true
								>
									<span class="mr-2">Set Library Folder</span>
									<v-icon>folder</v-icon>
								</v-btn>
							</v-flex>
						</v-layout> 
						
						<v-layout row pr-5 pt-5 pl-10>
							<v-flex md6>
								<v-checkbox id="multipleInterfaces" class="mr-3 mt-3" v-model="allowMultipleInterfaces" @click="multipleInterfaces" label="Allow Multiple Interfaces" value="true"></v-checkbox>
							</v-flex>
						</v-layout> 
						
						<h2 class="ml-3 mt-6">Login to Metaverse</h2>
						
						<v-card-text>
							<v-form>
								<v-text-field
									label="Username"
									name="username"
									prepend-icon="person"
									type="text"
								/>

								<v-text-field
									id="password"
									label="Password"
									name="password"
									prepend-icon="lock"
									type="password"
								/>
								
								<v-text-field
									label="Metaverse Server"
									name="metaverse"
									prepend-icon="mdi-earth"
									type="text"
								/>
							</v-form>
						</v-card-text>
						<v-card-actions>
							<v-spacer />
							<v-btn color="primary">Login</v-btn>
						</v-card-actions>
					</v-card>
				</v-row>
			</v-container>
		</v-content>
	</v-app>
</template>

<script>

export default {
	name: 'Settings',
	methods: {
		selectInterface: function() {
			const { ipcRenderer } = require('electron');
			ipcRenderer.send('setAthenaLocation');
		},
		setLibrary: function() {
			const { ipcRenderer } = require('electron');
			ipcRenderer.send('setLibraryFolder');
		},
		multipleInterfaces: function() {
			this.$store.commit('setAllowMultipleInterfaces', this.allowMultipleInterfaces);
		},
	},
	data: () => ({
		show: false,
		allowMultipleInterfaces: false,
		interfaceFolders: [
			{ folder: 'Click Me' },
			{ folder: 'Click Me' },
			{ folder: 'Click Me' },
			{ folder: 'Click Me 2' },
		],
	}),
};
</script>
