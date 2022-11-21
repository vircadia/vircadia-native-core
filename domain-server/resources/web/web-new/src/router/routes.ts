//  routes.ts
//
//  Created by Kalila L. on Sep. 4th, 2021.
//  Copyright 2021 Vircadia contributors.
//  Copyright 2021 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

import { RouteRecordRaw } from "vue-router";

const routes: RouteRecordRaw[] = [
    {
        path: "/",
        component: () => import("layouts/Index.vue"),
        children: [{ path: "", component: () => import("pages/Index.vue") }]
    },
    {
        path: "/wizard",
        component: () => import("layouts/FirstTimeWizard.vue"),
        children: [{ path: "", component: () => import("pages/FirstTimeWizard/Index.vue") }]
    },

    // routes for the various settings pages
    {
        path: "/networking",
        component: () => import("layouts/Index.vue"),
        children: [{ path: "", component: () => import("pages/Settings/Networking.vue") }]
    },
    {
        path: "/security",
        component: () => import("layouts/Index.vue"),
        children: [{ path: "", component: () => import("pages/Settings/Security.vue") }]
    },
    {
        path: "/content",
        component: () => import("layouts/Index.vue"),
        children: [{ path: "", component: () => import("pages/Settings/Content.vue") }]
    },
    {
        path: "/audio",
        component: () => import("layouts/Index.vue"),
        children: [{ path: "", component: () => import("pages/Settings/Audio.vue") }]
    },
    {
        path: "/avatars",
        component: () => import("layouts/Index.vue"),
        children: [{ path: "", component: () => import("pages/Settings/Avatars.vue") }]
    },
    {
        path: "/scripts",
        component: () => import("layouts/Index.vue"),
        children: [{ path: "", component: () => import("pages/Settings/Scripts.vue") }]
    },
    {
        path: "/advanced",
        component: () => import("layouts/Index.vue"),
        children: [{ path: "", component: () => import("pages/Settings/Advanced.vue") }]
    },
    {
        path: "/backup-restore",
        component: () => import("layouts/Index.vue"),
        children: [{ path: "", component: () => import("pages/Settings/Backup-Restore.vue") }]
    },
    {
        path: "/assignment",
        component: () => import("layouts/Index.vue"),
        children: [{ path: "", component: () => import("pages/Assignment/Index.vue") }]
    },
    {
        path: "/help",
        component: () => import("layouts/Index.vue"),
        children: [{ path: "", component: () => import("pages/Help/Index.vue") }]
    },

    // Always leave this as last one,
    // but you can also remove it
    {
        path: "/:catchAll(.*)*",
        component: () => import("pages/Error404.vue")
    }
];

export default routes;
