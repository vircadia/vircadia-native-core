import Log from "@Modules/debugging/log";
import { doAPIGet, findErrorMsg } from "src/modules/utilities/apiHelpers";
import { SettingsResponse, SettingsValues } from "./interfaces/settings";

export const Settings = {

    async getAll (): Promise<SettingsValues> {
        let response: SettingsValues = {};

        try {
            const apiRequestUrl = "settings.json";
            const settingsResponse = await doAPIGet(apiRequestUrl) as SettingsResponse;

            response = settingsResponse.values;

            return response;
        } catch (err) {
            const errr = findErrorMsg(err);
            Log.error(Log.types.API, `Exception while attempting to get settings: ${errr}`);
        }

        return response;
    }

};
