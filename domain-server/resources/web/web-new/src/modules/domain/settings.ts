// edited 24/11/2022 by Ujean

// import Log from "@Modules/debugging/log";
import { doAPIGet, findErrorMsg } from "src/modules/utilities/apiHelpers";
import { SettingsResponse, SettingsValues, Description } from "./interfaces/settings";
import Log from "../../modules/utilities/log";

const axios = require("axios");

export const Settings = {
    async getValues (): Promise<SettingsValues> {
        let response: SettingsValues = {};
        try {
            const apiRequestUrl = "settings.json";
            const settingsResponse = await doAPIGet(apiRequestUrl) as SettingsResponse;

            response = settingsResponse.values;
            return response;
        } catch (error) {
            const errorMessage = findErrorMsg(error);
            console.log(errorMessage);
            // Log.error(Log.types.API, `Exception while attempting to get settings: ${errr}`);
        }
        return response;
    },
    async getDescriptions (): Promise<Description[]> {
        let response: Description[] = [];
        try {
            const apiRequestUrl = "settings.json";
            const settingsResponse = await doAPIGet(apiRequestUrl) as SettingsResponse;

            response = settingsResponse.descriptions;
            return response;
        } catch (error) {
            const errorMessage = findErrorMsg(error);
            console.log(errorMessage);
            // Log.error(Log.types.API, `Exception while attempting to get settings: ${errr}`);
        }
        return response;
    },
    commitSettings (settingsToCommit: any) {
        return axios.post("/settings.json", JSON.stringify(settingsToCommit))
            .then(() => {
                Log.info(Log.types.DOMAIN, "Successfully committed settings.");
                return true;
            })
            .catch((response: string) => {
                Log.error(Log.types.DOMAIN, `Failed to commit settings to Domain: ${response}`);
                return false;
            });
    }
};
