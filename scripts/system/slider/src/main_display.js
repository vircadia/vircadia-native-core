import Vue from 'vue'
import Display from './Display.vue'
import vuetify from './plugins/vuetify';

Vue.config.productionTip = false

window.$ = window.jQuery = require('jquery')

new Vue({
  vuetify,
  render: h => h(Display)
}).$mount('#app')
