
<template>
  <q-layout view="hHh lpR fFf">
    <q-dialog v-model="confirmRestart" persistent>
      <q-card>
          <q-card-section class="row items-center">
            <q-avatar icon="mdi-alert" text-color="warning" size="40px" font-size="40px"/>
            <span class="q-ml-sm">Are you sure?</span>
          </q-card-section>
          <q-card-section class="row items-center">
            <span class="q-ml-sm">This will restart your domain server, causing your domain to be briefly offline.</span>
          </q-card-section>
          <q-card-actions align="center">
              <q-btn flat label="Cancel" color="primary" v-close-popup />
              <q-btn flat label="Restart" color="primary" @click="restartServer" v-close-popup />
          </q-card-actions>
      </q-card>
    </q-dialog>
    <q-header elevated class="bg-blue-grey-10 text-white">
      <q-toolbar>
        <q-btn dense flat round icon="menu" @click="toggleLeftDrawer" />

        <q-toolbar-title>
          <q-avatar>
            <img src="icons/favicon-128x128.png">
          </q-avatar>
          Domain Server Administration
        </q-toolbar-title>

        <q-space />

        <q-btn
            flat
            dense
            icon="save"
            aria-label="Save Settings"
            label="Save Settings"
            @click="saveSettings"
            class="q-mr-sm q-ml-sm"
        />
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
import { ref } from "vue";
import DrawerMenu from "../components/navigation/DrawerMenu.vue";

export default {
    components: {
        DrawerMenu
    },
    setup () {
        const leftDrawerOpen = ref(false);

        return {
            leftDrawerOpen,
            toggleLeftDrawer () {
                leftDrawerOpen.value = !leftDrawerOpen.value;
            },
            confirmRestart: ref(false)
        };
    },
    methods: {
        async restartServer () {
            // TODO: add a confirmation question here
            alert("restart");
        }
    }
};
</script>
