<template>
    <div class="q-pa-sm">
        <Description/>
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import Description from "@Components/settings/Description.vue";

export default defineComponent({
    name: "Index",
    components: {
        Description
    }
});
</script>
