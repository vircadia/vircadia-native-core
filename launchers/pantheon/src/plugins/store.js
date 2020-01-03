import Vue from 'vue';
import Vuex from 'vuex';

Vue.use(Vuex);

export const store = new Vuex.Store({
	state: {
		allowMultipleInterfaces: false,
		populatedInterfaceList: [],
	},
	mutations: {
		setAllowMultipleInterfaces(state, bool) {
			state.allowMultipleInterfaces = bool;
		},
		populateInterfaceList(state, array) {
			state.populatedInterfaceList = array;
		}
	}
})