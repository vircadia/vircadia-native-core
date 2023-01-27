export interface Column {
        label: string;
        name: string;
        placeholder: string;
        default?: number;
        "can_set"?: boolean;
    }

export interface Key {
        label: string;
        name: string;
        placeholder: string;
    }

export interface Setting {
        "can_add_new_rows": boolean;
        columns: Column[];
        "content_setting": boolean;
        help: string;
        key: Key;
        label: string;
        name: string;
        type: string;
        advanced?: boolean;
        // eslint-disable-next-line @typescript-eslint/no-explicit-any
        default: any;
        placeholder: string;
        numbered?: boolean;
        "can_order"?: boolean;
    }

export interface Description {
        "html_id": string;
        label: string;
        restart: boolean;
        settings: Setting[];
        name: string;
        "assignment-types": number[];
        hidden?: boolean;
    }

export interface AttenuationCoefficient {
        coefficient: string;
        listener: string;
        source: string;
    }

export interface Reverb {
        "reverb_time": string;
        "wet_level": string;
        zone: string;
    }

export interface Zone {
        "x_max": string;
        "x_min": string;
    }

export interface AudioEnv {
        "attenuation_coefficients": AttenuationCoefficient[];
        "attenuation_per_doubling_in_distance": string;
        reverb: Reverb[];
        zones: Record<string, Zone>;
    }

export interface EntityServerSettings {
        entityEditFilter: string;
    }

export interface InstalledContent {
        "creation_time": number;
        filename: string;
        "install_time": number;
        "installed_by": string;
        name: string;
    }

export interface Path {
        viewpoint: string;
    }

export interface PersistentScript {
        "num_instances": string;
        pool: string;
        url: string;
    }

export interface Scripts {
        "persistent_scripts": PersistentScript[];
    }

export interface ContentSettingsValues {
        "audio_env"?: AudioEnv;
        "entity_server_settings"?: EntityServerSettings;
        "installed_content"?: InstalledContent;
        paths?: Record<string, Path>;
        scripts?: Scripts;
        version?: number;
    }

export interface ContentSettingsResponse {
        descriptions: Description[];
        values: ContentSettingsValues;
    }

export interface PathsSaveSetting {
    paths: Record<string, Path>;
}

export interface ScriptsSaveSetting {
    "scripts": {
        "persistent_scripts": PersistentScript[];
    }
}
