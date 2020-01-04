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
							
							<v-btn
								color="purple"
								:tile=true
								v-on:click.native="populateInterfaceList()"
							>
								<v-icon>mdi-refresh</v-icon>
							</v-btn>
							
							<v-menu
								transition="slide-y-transition"
								bottom
							>
								<template v-slot:activator="{ on }">
									<v-btn
										:tile=true
										v-on="on"
									>
									Set Client & Profile
									</v-btn>
								</template>
								<v-list>
									<v-list-item
										v-for="(item, i) in interfaceFolders"
										:key="i"
										@click="selectInterface(item)"
									>
									<v-list-item-title>{{ item.name }}</v-list-item-title>
									</v-list-item>
								</v-list>
							</v-menu>
							
						</v-toolbar>
						
						<h2 class="ml-3 mt-3">General</h2>
						
						<v-layout row pr-5 pt-5 pl-10>
							<v-flex md6>
								<v-btn 
									v-on:click.native="selectInterfaceExe()"
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
								<v-checkbox color="blue" id="multipleInterfaces" class="mr-3 mt-3" v-model="allowMultipleInterfaces" @click="multipleInterfaces" label="Allow Multiple Interfaces" value="true"></v-checkbox>
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
							<v-btn disabled color="primary">Login</v-btn>
						</v-card-actions>
					</v-card>
				</v-row>
			</v-container>
		</v-content>
		<v-dialog
			width="500"
			v-model="showRequireInterface"
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
					Please select an interface by clicking "Set Client & Profile". If the list is empty, select a library. If there is nothing in your library, download Athena to it.
				</v-card-text>
		
				<v-divider></v-divider>
		
				<v-card-actions>
					<v-spacer></v-spacer>
					<v-btn
						color="primary"
						text
						@click="showRequireInterface = false"
					>
						Okay
					</v-btn>
				</v-card-actions>
			</v-card>
		</v-dialog>
	</v-app>
</template>

<script>
var store_p;
var vue_this;

const { ipcRenderer } = require('electron');
ipcRenderer.on('interface-list', (event, arg) => {
	vue_this.interfaceFolders = [];
	store_p.commit('populateInterfaceList', arg);
	var populatedList = store_p.state.populatedInterfaceList;
	populatedList.forEach(function(i){
		var appName = Object.keys(i)[0];
		var appLoc = i[appName].location;
		var appObject = { "name": appName, "folder": appLoc };
		vue_this.interfaceFolders.push(appObject);
		// console.info(i);
		// console.info(Object.keys(i)[0]);
		// console.info(appLoc);
	});
});

ipcRenderer.on('interface-selection-required', (event, arg) => {
	console.info(arg);
});
export default {	
	name: 'Settings',
	methods: {
		selectInterface: function(selected) {
			this.interfaceSelectionRequired = false;
			const { ipcRenderer } = require('electron');			
			ipcRenderer.send('setCurrentInterface', selected.folder);
		},
		selectInterfaceExe: function() {
			if(this.interfaceSelectionRequired) {
				this.showRequireInterface = true;
			} else {
				const { ipcRenderer } = require('electron');
				ipcRenderer.send('setAthenaLocation');
			}
		},
		setLibrary: function() {
			const { ipcRenderer } = require('electron');
			ipcRenderer.send('setLibraryFolder');
		},
		populateInterfaceList: function() {
			const { ipcRenderer } = require('electron');
			ipcRenderer.invoke('populateInterfaceList');
		},
		multipleInterfaces: function() {
			this.$store.commit('setAllowMultipleInterfaces', this.allowMultipleInterfaces);
		},
		isInterfaceSelectionRequired: function() {
			
		},
	},
	data: () => ({
		show: false,
		showRequireInterface: false,
		allowMultipleInterfaces: false,
		interfaceFolders: [],
		selectedInterface: null,
		interfaceSelectionRequired: true,
	}),
	created: function () {
		store_p = this.$store;
		vue_this = this;
		this.populateInterfaceList();
	}
};
</script>
