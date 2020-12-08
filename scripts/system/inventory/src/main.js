/*
    main.js

    Created by Kalila L. on 7 Apr 2020
    Copyright 2020 Vircadia and contributors.
    
    Distributed under the Apache License, Version 2.0.
    See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
*/

import Vue from 'vue'
import App from './App.vue'
import vuetify from './plugins/vuetify';
import { store } from './plugins/store';

window.$ = window.jQuery = require('jquery');

window.$.ajaxPrefilter(function(options) {
    if (options.crossDomain && window.$.support.cors) {
        options.url = 'https://cors-anywhere.herokuapp.com/' + options.url;
    }
});

Vue.config.productionTip = false;

window.vm = new Vue({
    vuetify,
    store,
    render: h => h(App)
}).$mount('#app');
