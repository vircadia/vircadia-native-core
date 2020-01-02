import Vue from 'vue';
import Vuex from 'vuex';

Vue.use(Vuex);

export const store = new Vuex.Store({
  state: {
    allowMultipleInterfaces: false,
  },
  mutations: {
    setAllowMultipleInterfaces(state, bool) {
      state.allowMultipleInterfaces = bool;
    }
  }
})