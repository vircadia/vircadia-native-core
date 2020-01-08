import Vue from 'vue';
import Vuex from 'vuex';

Vue.use(Vuex);

export const store = new Vuex.Store({
	devtools: true,
	state: {
		selectedInterface: null,
		interfaceSelectionRequired: true,
		allowMultipleInstances: false,
		metaverseServer: "",
		populatedInterfaceList: [],
		noSteamVR: false,
	},
	mutations: {
		mutate(state, payload) {
			state[payload.property] = payload.with;
			console.info("Payload:", payload.property, "with:", payload.with, "state is now:", this.state);
			const { ipcRenderer } = require('electron');
			ipcRenderer.send('save-state', this.state);
		}
	}
})