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
                                style="display: none;"
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
                                        @mouseover="populateInterfaceList()"
									>
                                        Select Interface
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
						
                        <h3 class="mx-7 mt-5">Launch Settings</h3>
                        
						<v-layout row pr-5 pt-5 pl-12>
							<v-flex md6>
								<v-btn 
									v-on:click.native="selectInterfaceExe()"
									:right=true
									class=""
									:tile=true
								>
									<span class="mr-2">Select Interface Executable</span>
									<v-icon>settings_applications</v-icon>
								</v-btn>
							</v-flex>
						</v-layout>
                        
                        <v-layout row pr-5 pt-5 pl-12>
                            <v-flex md6>
                                <v-checkbox color="blue" id="multipleInterfaces" class="mr-3 mt-3" v-model="allowMultipleInstances" @click="multipleInstances" label="Allow Multiple Instances" value="true"></v-checkbox>
                            </v-flex>
                        </v-layout> 

                        
						<h3 class="mx-7 mt-5">Library</h3>
                        
                        <p class="mx-7 mt-3 bodyText">The library folder is the directory that all of your Athena installations are located.<br />
                            For example, if you had Athena installed to: <pre>C:\Program Files (x86)\Athena-K2-RC1</pre> then you would make your library folder: <pre>C:\Program Files (x86)</pre>
                        </p>
                        
						<v-layout row pr-5 pt-5 pl-12>
							<v-flex md6 style="text-align: center;">
                                <v-tooltip top>
                                    <template v-slot:activator="{ on }">
                                        <v-btn
                                            v-on:click.native="setLibrary()"
                                            :right=true
                                            class=""
                                            :tile=true
                                            v-on="on"
                                        >
                                            <span class="mr-2">Set Library Folder</span>
                                            <v-icon>folder</v-icon>
                                        </v-btn>
                                    </template>
                                    <span>{{ currentFolder }}</span>
                                </v-tooltip>
							</v-flex>
                            <v-flex md6 style="text-align: center;">
                                <v-btn
                                    v-on:click.native="setLibraryDefault()"
                                    :right=true
                                    class=""
                                    :tile=true
                                >
                                    <span class="mr-2">Default</span>
                                    <v-icon>folder</v-icon>
                                </v-btn>
                            </v-flex>
						</v-layout> 
						
						<h2 class="ml-3 mt-6">Metaverse</h2>
						
						<v-card-text>
							<v-form>
								<v-text-field
									label="Username"
									name="username"
									prepend-icon="person"
									type="text"
									disabled
								/>

								<v-text-field
									id="password"
									label="Password"
									name="password"
									prepend-icon="lock"
									type="password"
									disabled
								/>
								
								<v-text-field
									label="Metaverse Server"
									name="metaverse"
									prepend-icon="mdi-earth"
									type="text"
									v-model="metaverseServer"
								/>
							</v-form>
						</v-card-text>
						<v-card-actions>
							<v-spacer />
							<v-btn @click="setMetaverseServer()" color="primary">Save</v-btn>
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
					Please select an interface by clicking "Select Interface". If the list is empty, select a library folder then click the download button in the bottom right.
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
    if (vue_this) { // Make sure this page is open!
        vue_this.interfaceFolders = [];
        store_p.commit('mutate', {
            property: 'populatedInterfaceList', 
            with: arg
        });
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
    }
});

ipcRenderer.on('interface-selection-required', (event, arg) => {
	console.info(arg);
});

export default {	
	name: 'Settings',
	methods: {
		selectInterface: function(selected) {
			this.$store.commit('mutate', {
				property: 'interfaceSelectionRequired', 
				with: false
			});
			this.$store.commit('mutate', {
				property: 'selectedInterface', 
				with: { name: selected.name, folder: selected.folder }
			});
			ipcRenderer.send('setCurrentInterface', selected.folder);
		},
		selectInterfaceExe: function() {
			if(this.$store.state.interfaceSelectionRequired) {
				this.showRequireInterface = true;
			} else {
				ipcRenderer.send('set-athena-location');
			}
		},
		setLibrary: function() {
			ipcRenderer.send('setLibraryFolder');
		},
        setLibraryDefault: function() {
            ipcRenderer.send('set-library-folder-default');
        },
        getLibraryFolder: function() {
			ipcRenderer.send('getLibraryFolder');
        },
		populateInterfaceList: function() {
            if(this.debounce()) {
                ipcRenderer.invoke('populateInterfaceList');
            }
		},
		multipleInstances: function() {
			this.$store.commit('mutate', {
				property: 'allowMultipleInstances', 
				with: this.allowMultipleInstances
			});
		},
		setMetaverseServer: function() {
			this.$store.commit('mutate', {
				property: 'metaverseServer', 
				with: this.metaverseServer
			});
			ipcRenderer.send('set-metaverse-server', this.$store.state.metaverseServer);
		},
        debounce: function() {
            if(this.readyToUseAgain) {
                console.log("Ready.");
                this.readyToUseAgain = false;
                setTimeout(function() {
                    vue_this.readyToUseAgain = true;
                }, 2000); // 2000ms before firing again.
                return true;
            } else {
                console.log("Not ready.");
                return false;
            }
        }
	},
	data: () => ({
		show: false,
		showRequireInterface: false,
		allowMultipleInstances: false,
		interfaceFolders: [],
		metaverseServer: "metaverse.highfidelity.com", // Default metaverse API URL
        currentFolder: "",
        readyToUseAgain: true,
	}),
    computed: {
        librarySelected () {
            return this.$store.state.currentLibraryFolder;
        }
    },
    watch: {
        librarySelected (newVal, oldVal) {
            this.currentFolder = newVal;
        }
    },
	created: function () {
        store_p = this.$store;
        vue_this = this;
        const { ipcRenderer } = require('electron');
        
		this.allowMultipleInstances = this.$store.state.allowMultipleInstances;
		this.populateInterfaceList();
        
		if (this.$store.state.metaverseServer) {
			this.metaverseServer = this.$store.state.metaverseServer;
		}
        
        this.setMetaverseServer();
        this.currentFolder = this.$store.state.currentLibraryFolder;
	}
};
</script>
