
<template>
  <q-layout view="hHh lpR fFf">

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
            @click="restartServer"
            class="q-mr-sm q-ml-sm"
        />
      </q-toolbar>
    </q-header>

    <q-drawer show-if-above v-model="leftDrawerOpen" side="left" bordered>
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
            }
        };
    }
};
</script>
