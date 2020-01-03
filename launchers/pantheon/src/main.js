import Vue from 'vue'
import App from './App.vue'
import vuetify from './plugins/vuetify';
import { store } from './plugins/store';
import 'roboto-fontface/css/roboto/roboto-fontface.css'
import '@mdi/font/css/materialdesignicons.css'
import 'material-design-icons-iconfont/dist/material-design-icons.css'

Vue.config.productionTip = false

new Vue({
	vuetify,
	store,
	render: h => h(App)
}).$mount('#app')