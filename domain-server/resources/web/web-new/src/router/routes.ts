import { RouteRecordRaw } from 'vue-router'

const routes: RouteRecordRaw[] = [
    {
        path: '/',
        component: () => import('pages/Index.vue'),
        children: [{ path: '', component: () => import('pages/Index.vue') }]
    },
    {
        path: '/wizard',
        component: () => import('layouts/FirstTimeWizard.vue'),
        children: [{ path: '', component: () => import('pages/FirstTimeWizard/Index.vue') }]
    },

    // Always leave this as last one,
    // but you can also remove it
    {
        path: '/:catchAll(.*)*',
        component: () => import('pages/Error404.vue')
    }
]

export default routes
