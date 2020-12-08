/*
    vue.config.js

    Created by Kalila L. on 7 Apr 2020
    Copyright 2020 Vircadia and contributors.
    
    Distributed under the Apache License, Version 2.0.
    See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
*/

module.exports = {
    publicPath: "./",
    assetsDir: "./",
    "transpileDependencies": [
        "vuetify"
    ],
    configureWebpack: {
        devServer: {
            headers: {
                "Access-Control-Allow-Origin": "*",
                "Access-Control-Allow-Methods": "GET, POST, PUT, DELETE, PATCH, OPTIONS",
                "Access-Control-Allow-Headers": "X-Requested-With, content-type, Authorization"
            }
        }
    }
}
