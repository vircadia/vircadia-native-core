import Vue from 'vue'
import App from './App.vue'
import vuetify from './plugins/vuetify';

Vue.config.productionTip = false

window.$ = window.jQuery = require('jquery')
window.$arrayMove = require('array-move')

new Vue({
  vuetify,
  render: h => h(App)
}).$mount('#app')
