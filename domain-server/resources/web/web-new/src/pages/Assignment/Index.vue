<template>
    <q-page class="q-pa-md row items-stretch">
    <q-card class="col-12">
        <iframe :src="assignmentUrl" width="100%" height="100%" frameborder="0"></iframe>
    </q-card>
    </q-page>
</template>

<script lang="ts">
import { defineComponent } from "vue";

export default defineComponent({
    name: "Index",

    data () {
        return {
            assignmentUrl: "http://localhost:40100/assignment/"
        };
    }
});
</script>
