
<template>
  <q-layout view="hHh lpR fFf">
    <q-dialog v-model="confirmRestart" persistent>
      <q-card class="bg-primary q-pa-md">
          <q-card-section class="row items-center">
            <q-avatar icon="mdi-alert" text-color="warning" size="28px" font-size="28px"/>
            <span class="text-h5 q-ml-sm text-bold text-white">Confirm Restart?</span>
            <span class="text-body2 q-mt-sm">Your domain server will be briefly offline while it restarts</span>
          </q-card-section>
          <q-card-actions align="center">
              <q-btn flat label="Cancel" v-close-popup />
              <q-btn flat label="Restart" @click="restartServer" v-close-popup />
          </q-card-actions>
      </q-card>
    </q-dialog>
    <q-dialog v-model="restartPopup" persistent transition-duration="200" transition-hide="fade">
      <q-card flat class="transparent q-pa-md">
        <q-card-actions align="center" vertical class="row items-center no-wrap">
            <q-spinner color="accent" size="4rem" thickness="8"/>
            <p class="q-mt-md text-h5">Server Restarting...</p>
            <q-linear-progress :value="restartProgress" animation-speed="500" color="accent"/>
        </q-card-actions>
      </q-card>
    </q-dialog>
    <q-header elevated class="primary-10 text-white">
      <q-toolbar>
        <q-btn dense flat round icon="menu" @click="toggleLeftDrawer" />

        <q-toolbar-title>
          <q-avatar size="1.1em" class="q-mb-xs">
            <img src="icons/favicon-128x128.png">
          </q-avatar>
          <span class="text-h6 q-ml-sm gt-xs">Domain Server Administration</span>
        </q-toolbar-title>

        <!-- <q-btn
            flat
            dense
            icon="save"
            aria-label="Save Settings"
            label="Save Settings"
            @click="saveSettings"
            class="q-mr-sm q-ml-sm"
        /> -->
        <q-btn
            flat
            dense
            icon="mdi-restart"
            aria-label="Restart Server"
            label="Restart Server"
            @click="confirmRestart = true"
            class="q-mr-sm q-ml-sm"
        />
      </q-toolbar>
    </q-header>

    <q-drawer
      show-if-above
      v-model="leftDrawerOpen"
      side="left"
      bordered
      :width="220"
      >
      <DrawerMenu></DrawerMenu>
    </q-drawer>

    <q-page-container>
      <router-view />
    </q-page-container>

  </q-layout>
</template>

<script>
import DrawerMenu from "../components/navigation/DrawerMenu.vue";
import { doAPIGet } from "../modules/utilities/apiHelpers";

export default {
    components: {
        DrawerMenu
    },
    data () {
        return {
            restartPopup: false,
            restartProgress: 0,
            confirmRestart: false,
            leftDrawerOpen: false
        };
    },
    methods: {
        async restartServer () { // TODO: Make 3000 ms delay into a variable constant
            const myPromise = new Promise(function (resolve) {
                const apiRequestUrl = "restart";
                doAPIGet(apiRequestUrl);
                setTimeout(function () { resolve("Domain Server Restarted"); }, 3000);
            });
            // TODO: make progress bar dynamic (hardcoded to 3000 ms currently)
            const progressInterval = setInterval(() => { this.restartProgress += 0.05; }, 3000 / 23.5); // updates linear progress bar
            this.restartPopup = true;
            await myPromise;
            clearInterval(progressInterval);
            this.restartProgress = 0;
            this.restartPopup = false;
        },
        toggleLeftDrawer () {
            this.leftDrawerOpen = !this.leftDrawerOpen;
        }
    }
};
</script>
