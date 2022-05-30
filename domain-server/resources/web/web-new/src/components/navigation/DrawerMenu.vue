<template>
    <div>
    <q-list >
      <q-item v-for="menuItem in menuItems" :key="menuItem.route" :to="menuItem.route" exact>
        <q-item-section avatar>
          <q-icon :name="menuItem.icon" />
        </q-item-section>
        <q-item-section>{{ menuItem.label }}</q-item-section>
      </q-item>
    </q-list>
    </div>
</template>

<script lang="ts">

export default {
    data () {
        return {
            menuItems: [
                { icon: "mdi-view-dashboard", route: "/", label: "Dashboard" },
                { icon: "mdi-lan", route: "/networking", label: "Networking" },
                { icon: "mdi-shield-lock-outline", route: "/security", label: "Security" },
                { icon: "mdi-cube-outline", route: "/content", label: "Content" },
                { icon: "mdi-volume-high", route: "/audio", label: "Audio" },
                { icon: "mdi-script-text-outline", route: "/scripts", label: "Server Scripts" },
                { icon: "mdi-cog-outline", route: "/advanced", label: "Advanced" },
                { icon: "mdi-backup-restore", route: "/backup-restore", label: "Backup / Restore" },
                { icon: "mdi-console", route: "/assignment", label: "Assignment" },
                { icon: "mdi-help-circle", route: "/help", label: "Help / Troubleshooting" }
            ]
        };
    }
};

</script>
