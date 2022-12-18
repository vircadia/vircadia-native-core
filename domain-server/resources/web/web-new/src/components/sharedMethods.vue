
<template>
    <!-- RESTARTING SERVER LOADING POPUP  -->
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

<script lang="ts">
import { MetaverseSaveSettings, WebrtcSaveSettings, WordpressSaveSettings, SSLClientAcmeSaveSettings, MonitoringSaveSettings, SecuritySaveSettings } from "../modules/domain/interfaces/settings";
import { doAPIGet } from "../modules/utilities/apiHelpers";
import Log from "../modules/utilities/log";

const axios = require("axios");
const timers: number[] = [];

export default {
    name: "SharedMethods",
    props: {
        restartServer: Boolean,
        automaticCommitSettings: Object
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
        },
        automaticCommitSettings (settingsToCommit: MetaverseSaveSettings | WebrtcSaveSettings | WordpressSaveSettings | SSLClientAcmeSaveSettings | MonitoringSaveSettings | SecuritySaveSettings): void {
            // automaticCommitSettings should be called whenever an input change is detected
            // only commits changes once no input changes are detected for 5 secs (5000 ms)
            // call commitSettings instead of automaticCommitSettings to instantly commit changes
            timers.forEach((timerID, index) => { clearTimeout(timerID); timers.splice(index, 1); });
            timers.push(window.setTimeout(() => this.commitSettings(settingsToCommit), 5000));
        }
    },
    methods: {
        commitSettings (settingsToCommit: MetaverseSaveSettings | WebrtcSaveSettings | WordpressSaveSettings | SSLClientAcmeSaveSettings | MonitoringSaveSettings | SecuritySaveSettings) {
            console.log("CALLED COMMIT SETTINGS");
            return axios.post("/settings.json", JSON.stringify(settingsToCommit))
                .then(() => {
                    Log.info(Log.types.DOMAIN, "Successfully committed settings.");
                    console.log("SUCCESSFULLY COMMITTED SETTINGS");
                    return true;
                })
                .catch((response: string) => {
                    Log.error(Log.types.DOMAIN, `Failed to commit settings to Domain: ${response}`);
                    return false;
                });
        }
    }
};
</script>
