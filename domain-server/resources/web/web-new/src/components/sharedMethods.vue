
<template>
    <q-dialog v-model="restartPopup" persistent transition-duration="200" transition-hide="fade">
      <q-card flat class="transparent q-pa-md">
        <q-card-actions align="center" vertical class="row items-center no-wrap">
            <q-spinner color="accent" size="4rem" thickness="8"/>
            <p class="q-mt-md text-h5">Server Restarting...</p>
            <q-linear-progress :value="restartProgress" animation-speed="500" color="accent"/>
        </q-card-actions>
      </q-card>
    </q-dialog>
</template>

<script>
import { doAPIGet } from "../modules/utilities/apiHelpers";

export default {
    name: "SharedMethods",
    props: {
        restartServer: Boolean
    },
    data () {
        return {
            restartPopup: false,
            restartProgress: 0
        };
    },
    watch: {
        restartServer: async function () {
            // TODO: Make 3000 ms delay into a variable constant
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
        }
    }
};
</script>
