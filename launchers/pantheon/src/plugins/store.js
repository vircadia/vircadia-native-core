import Vue from 'vue';
import Vuex from 'vuex';

Vue.use(Vuex);

export const store = new Vuex.Store({
	state: {
		allowMultipleInterfaces: false,
		populatedInterfaceList: [],
		noSteamVR: false,
	},
	mutations: {
		setAllowMultipleInterfaces(state, bool) {
			state.allowMultipleInterfaces = bool;
		},
		setSteamVR(state, bool) {
			state.noSteamVR = bool;
		},
		populateInterfaceList(state, array) {
			state.populatedInterfaceList = array;
		},
	}
})