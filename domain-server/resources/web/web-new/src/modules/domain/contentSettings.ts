// edited 24/11/2022 by Ujean

// import Log from "@Modules/debugging/log";
import { doAPIGet, findErrorMsg } from "src/modules/utilities/apiHelpers";
import { ContentSettingsResponse, ContentSettingsValues } from "./interfaces/contentSettings";

export const ContentSettings = {

    async getAll (): Promise<ContentSettingsValues> {
        let response: ContentSettingsValues = {};

        try {
            const apiRequestUrl = "content-settings.json";
            const contentSettingsResponse = await doAPIGet(apiRequestUrl) as ContentSettingsResponse;

            response = contentSettingsResponse.values;

            return response;
        } catch (error) {
            const errorMessage = findErrorMsg(error);
            console.log(`Exception while attempting to get content-settings: ${errorMessage}`);
            // Log.error(Log.types.API, `Exception while attempting to get content-settings: ${errr}`);
        }

        return response;
    }

};
