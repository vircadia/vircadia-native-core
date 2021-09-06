//  axios.ts
//
//  Created by Kalila L. on Sep. 4th, 2021.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

import { boot } from "quasar/wrappers";
import axios, { AxiosInstance } from "axios";
import Log from "../modules/utilities/log";

declare module "@vue/runtime-core" {
    interface ComponentCustomProperties {
        $axios: AxiosInstance;
    }
}

Log.info(Log.types.OTHER, "Bootstrapping Axios.");

axios.defaults.headers.common = {
    "x-vircadia-error-handle": "badrequest"
};

// Be careful when using SSR for cross-request state pollution
// due to creating a Singleton instance here;
// If any client changes this (global) instance, it might be a
// good idea to move this instance creation inside of the
// "export default () => {}" function below (which runs individually
// for each client)
const api = axios.create({ baseURL: "https://api.example.com" });

export default boot(({ app }) => {
    // for use inside Vue files (Options API) through this.$axios and this.$api

    app.config.globalProperties.$axios = axios;
    // ^ ^ ^ this will allow you to use this.$axios (for Vue Options API form)
    //       so you won't necessarily have to import axios in each vue file

    app.config.globalProperties.$api = api;
    // ^ ^ ^ this will allow you to use this.$api (for Vue Options API form)
    //       so you can easily perform requests against your app's API
});

export { api };
