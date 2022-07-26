
import { ContentSettingsValues } from "@Base/modules/domain/interfaces/contentSettings";
import { SettingsValues } from "@Base/modules/domain/interfaces/settings";

export interface MainState {
    serverSettings: {
        ContentSettings: ContentSettingsValues,
        Settings: SettingsValues
    },
    workingSettings: {
        ContentSettings: ContentSettingsValues,
        Settings: SettingsValues
    },
}

function state (): MainState {
    return {
        serverSettings: {
            ContentSettings: {},
            Settings: {}
        },
        workingSettings: {
            ContentSettings: {},
            Settings: {}
        }
    };
}

export default state;
