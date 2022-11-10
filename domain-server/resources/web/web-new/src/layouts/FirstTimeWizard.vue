<!--
//  FirstTimeWizard.vue
//
//  Created by Kalila L. on Sep. 4th, 2021.
//  Copyright 2021 Vircadia contributors.
//  Copyright 2021 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <q-layout id="vantaBG" view="hHh lpR fFf">
        <q-page-container>
            <div id="firstTimeWizardContainer">
                <div id="bgFreezeCheckboxContainer">
                    <q-checkbox
                        v-model="bgFreezeCheckbox"
                        label="Freeze Background"
                        left-label
                        dark
                        @update:model-value = "bgFreezeCheckboxUpdate"
                    />
                </div>
                <router-view />
            </div>
        </q-page-container>
    </q-layout>
</template>

<script>
import { defineComponent, ref } from "vue";

import * as THREE from "three";

export default defineComponent({
    name: "FirstTimeWizard",
    setup () {
        return {
            bgFreezeCheckbox: ref(false)
        };
    },
    data () {
        return {
            vantaBG: null,
            vantaRings: null,
            refreshVantaTimeout: null,
            DELAY_REFRESH_VANTA: 500
        };
    },
    async mounted () {
        window.THREE = THREE;
        this.vantaRings = (await import("vanta/dist/vanta.rings.min")).default;

        this.initVanta();

        visualViewport.addEventListener("resize", this.onResize);
    },
    methods: {
        bgFreezeCheckboxUpdate (value) {
            if (this.vantaBG) {
                if (value) {
                    if (this.vantaBG.req) {
                        window.cancelAnimationFrame(this.vantaBG.req);
                        this.vantaBG.req = null;
                    }
                } else {
                    if (this.vantaBG.req === null) {
                        this.vantaBG.req = window.requestAnimationFrame(this.vantaBG.animationLoop);
                    }
                }
            }
        },
        onResize () {
            if (this.refreshVantaTimeout) {
                clearTimeout(this.refreshVantaTimeout);
            }

            this.refreshVantaTimeout = setTimeout(() => {
                this.initVanta();

                // force single run of animation loop
                // otherwise it looks broken if cancelled immediately after initialization
                window.cancelAnimationFrame(this.vantaBG.req);
                this.vantaBG.animationLoop();

                this.bgFreezeCheckboxUpdate(this.bgFreezeCheckbox);

                this.refreshVantaTimeout = null;
            }, this.DELAY_REFRESH_VANTA);
        },
        initVanta () {
            if (this.vantaBG) {
                this.vantaBG.destroy();
            }

            this.vantaBG = this.vantaRings({
                el: "#vantaBG",
                mouseControls: false,
                touchControls: false,
                gyroControls: false,
                minHeight: 200.00,
                minWidth: 200.00,
                scale: 1.00,
                scaleMobile: 1.00,
                color: 0x000000
                // waveHeight: 20,
                // shininess: 50,
                // waveSpeed: 1.5,
                // zoom: 0.75,
                // backgroundColor: 0x4e97e1,
                // backgroundAlpha: 0.40
            });
        }
    },
    beforeUnmount () {
        if (this.vantaBG) {
            this.vantaBG.destroy();
        }
    }
});
</script>
