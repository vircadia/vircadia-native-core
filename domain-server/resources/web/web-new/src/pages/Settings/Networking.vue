<template>
    <div class="q-pa-md">
        <q-card class="my-card">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold">Your Metaverse Account</div>
                <div v-if="isUserConnected" class="text-overline text-positive text-center">Metaverse Account Connected</div>
                <div v-else class="text-overline text-negative text-center">Account Not Connected</div>
                <q-separator inset />

                <!-- METAVERSE LOGIN/LOGOUT (connect/disconnect) BUTTON -->
                <q-card-actions align="center">
                    <q-btn v-if="!isUserConnected" class="q-mt-sm" padding="0.5em 2em" push color="positive" label="Connect Metaverse Account" />
                    <q-btn v-else class="q-mt-sm" padding="0.5em 1.5em" push color="negative" label="Disconnect Account" />
                </q-card-actions>
            </q-card-section>
        </q-card>
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";

export default defineComponent({
    name: "Index",

    data () {
        return {
            isUserConnected: false
        };
    }
});
</script>
