<template>
    <div>
    <q-list >
      <q-item v-for="menuItem in menuItems" :key="menuItem.route" :to="menuItem.route" exact active-class="bg-grey-8 text-accent active-tab">
        <q-item-section avatar>
          <q-icon :name="menuItem.icon">
          </q-icon>

        </q-item-section>
        <q-item-section>
          <span class="relative-position">
            {{ menuItem.label }}
            <q-badge v-show="menuItem.badge.length > 0" color="primary" :label="menuItem.badge" floating rounded/>
          </span>
        </q-item-section>
      </q-item>
    </q-list>
    </div>
</template>

<script lang="ts">

export default {
    data () {
        return {
            menuItems: [
                { icon: "mdi-view-dashboard", route: "/", label: "Dashboard", badge: "" },
                { icon: "mdi-lan", route: "/networking", label: "Networking", badge: "!" },
                { icon: "mdi-shield-lock-outline", route: "/security", label: "Security", badge: "" },
                { icon: "mdi-cube-outline", route: "/content", label: "Content", badge: "" },
                { icon: "mdi-volume-high", route: "/audio", label: "Audio", badge: "" },
                { icon: "mdi-account-multiple", route: "/avatars", label: "Avatars", badge: "" },
                { icon: "mdi-account-outline", route: "/entities", label: "Entities", badge: "" },
                { icon: "mdi-script-text-outline", route: "/scripts", label: "Server Scripts", badge: "" },
                { icon: "mdi-cog-outline", route: "/advanced", label: "Advanced", badge: "" },
                { icon: "mdi-backup-restore", route: "/backup-restore", label: "Backup & Restore", badge: "" },
                { icon: "mdi-console", route: "/assignment", label: "Assignment", badge: "" },
                { icon: "mdi-help-circle", route: "/help", label: "Help & Troubleshooting", badge: "" }
            ]
        };
    }
};

</script>
