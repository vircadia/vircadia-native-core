<!--
//  ConnectMetaverse.vue
//
//  Created by Kalila L. on Sep. 5th, 2021.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <q-card-section>
        <div class="row no-wrap items-center">
            <div class="col text-h4 ellipsis">
                Metaverse
            </div>
        </div>

    </q-card-section>

    <q-separator />

    <q-card-section>
        <MetaverseLogin></MetaverseLogin>
    </q-card-section>
</template>

<script>
import MetaverseLogin from "../components/login/MetaverseLogin.vue";

export default {
    name: "ConnectMetaverse",

    components: {
        MetaverseLogin
    },

    data: () => ({
    }),

    methods: {
    }
};
</script>
