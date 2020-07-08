import Vue from 'vue'
import App from './App.vue'
import vuetify from './plugins/vuetify';
import { store } from './plugins/store';

Vue.config.productionTip = false

window.vm = new Vue({
  vuetify,
  store,
  render: h => h(App)
}).$mount('#app')
