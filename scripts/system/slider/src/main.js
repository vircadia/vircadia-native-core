//
//  main.js
//
//  Created by kasenvr@gmail.com on 11 Jul 2020
//  Copyright 2020 Vircadia and contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Vue from 'vue'
import App from './App.vue'
import vuetify from './plugins/vuetify';

Vue.config.productionTip = false

window.$ = window.jQuery = require('jquery')

window.vm = new Vue({
  vuetify,
  render: h => h(App)
}).$mount('#app')
