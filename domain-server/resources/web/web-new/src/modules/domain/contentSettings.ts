import Log from "@Modules/debugging/log";
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
        } catch (err) {
            const errr = findErrorMsg(err);
            Log.error(Log.types.API, `Exception while attempting to get content-settings: ${errr}`);
        }

        return response;
    }

};
