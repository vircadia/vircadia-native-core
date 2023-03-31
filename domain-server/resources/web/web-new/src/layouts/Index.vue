
<template>
  <SharedMethods :restart-server-watcher="restartServer"/>
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
              <q-btn flat label="Restart" @click="toggleRestartServer()" v-close-popup />
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
        <div v-if="!isConnected">
          <q-btn :to="'/networking'" class="text-warning" style="background-color: #2D7EB9;" aria-label="Not Connected" flat>NOT CONNECTED</q-btn>
        </div>
        <div v-else>
          <q-btn :to="'/networking'" class="text-positive" style="background-color: #1B7EB9;" aria-label="Connected" flat>CONNECTED</q-btn>
        </div>
        <q-btn
            flat
            dense
            icon="mdi-restart"
            aria-label="Restart Server"
            @click="confirmRestart = true"
            class="q-mr-sm q-ml-sm"
        ><span class="gt-xs">Restart Server</span></q-btn>
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
import SharedMethods from "@Components/sharedMethods.vue";
import { StoreInstance } from "../modules/store";

export default {
    components: {
        DrawerMenu,
        SharedMethods
    },
    data () {
        return {
            isConnected: StoreInstance.isLoggedIn,
            restartServer: false,
            confirmRestart: false,
            leftDrawerOpen: false
        };
    },
    methods: {
        toggleLeftDrawer () {
            this.leftDrawerOpen = !this.leftDrawerOpen;
        },
        toggleRestartServer () {
            // whenever restart-server state changes, the watcher in sharedMethods.vue will fire
            this.restartServer = !this.restartServer;
        }
    }
};
</script>
