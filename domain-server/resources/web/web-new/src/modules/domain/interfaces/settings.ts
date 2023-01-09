/* eslint-disable @typescript-eslint/no-explicit-any */
/* eslint-disable camelcase */
export interface Option {
        label: string;
        value: string;
    }

export interface Option2 {
        label: string;
        value: string;
    }

export interface Column {
        can_set: boolean;
        label: string;
        name: string;
        type: string;
        default: any;
        editable?: boolean;
        hidden?: boolean;
        readonly?: boolean;
        options: Option2[];
        placeholder: any;
        help: string;
    }

export interface Group {
        label: string;
        span: number;
    }

export interface Setting {
        advanced: boolean;
        backup: boolean;
        help: string;
        label: string;
        name: string;
        default: any;
        options: Option[];
        type: string;
        can_add_new_rows?: boolean;
        columns: Column[];
        numbered?: boolean;
        password_placeholder: string;
        "value-hidden"?: boolean;
        placeholder: any;
        caption: string;
        groups: Group[];
        "non-deletable-row-key": string;
        "non-deletable-row-values": string[];
        can_add_new_categories?: boolean;
        categorize_by_key: string;
        new_category_message: string;
        new_category_placeholder: string;
        deprecated?: boolean;
        "assignment-types": number[];
        hidden?: boolean;
        readonly?: boolean;
    }

export interface Description {
        label: string;
        name: string;
        settings: Setting[];
        restart?: boolean;
        help: string;
        "assignment-types": number[];
        show_on_enable?: boolean;
        hidden?: boolean;
    }

export interface Acme {
        account_key_path: string;
        certificate_authority_filename: string;
        certificate_directory: string;
        certificate_domains: string;
        certificate_filename: string;
        certificate_key_filename: string;
        challenge_handler_type: string;
        directory_endpoint: string;
        eab_kid: string;
        eab_mac: string;
        enable_client: boolean;
        zerossl_api_key: string;
        zerossl_rest_api: boolean;
    }

export interface AssetServer {
        assets_filesize_limit: number;
        assets_path: string;
        enabled: boolean;
    }

export interface AudioBuffer {
        dynamic_jitter_buffer: boolean;
        max_frames_over_desired: string;
        repetition_with_fade: string;
        static_desired_jitter_buffer_frames: string;
        use_stdev_for_desired_calc: string;
        window_seconds_for_desired_calc_on_too_many_starves: string;
        window_seconds_for_desired_reduction: string;
        window_starve_threshold: string;
    }

export interface AudioEnv {
        codec_preference_order: string;
        enable_filter: boolean;
        noise_muting_threshold: string;
    }

export interface AudioThreading {
        auto_threads: boolean;
        num_threads: string;
        throttle_backoff: number;
        throttle_start: number;
    }

export interface Authentication {
        enable_oauth2: boolean;
        oauth2_url_path: string;
        plugin_client_id: string;
        wordpress_url_base: string;
    }

export interface BackupRule {
        Name: string;
        backupInterval: string;
        maxBackupVersions: string;
    }

export interface AutomaticContentArchives {
        backup_rules: BackupRule[];
    }

export interface AvatarMixer {
        auto_threads: boolean;
        connection_rate: string;
        max_node_send_bandwidth: number;
        num_threads: string;
        priority_fraction: string;
    }

export interface Avatars {
        avatar_whitelist: string;
        max_avatar_height: number;
        min_avatar_height: number;
        replacement_avatar: string;
    }

export interface DownstreamServer {
        address: string;
        port: string;
        server_type: string;
    }

export interface UpstreamServer {
        address: string;
        port: string;
        server_type: string;
    }

export interface Broadcasting {
        downstream_servers: DownstreamServer[];
        upstream_servers: UpstreamServer[];
        users: string[];
    }

export interface Descriptors {
        contact_info: string;
        description: string;
        images: string[];
        managers: string[];
        maturity: string;
        tags: string[];
        thumbnail: string;
        world_name: string;
    }

export interface DomainServer {
        network_address: string;
        network_port: number;
    }

export interface EntityScriptServer {
        entity_pps_per_script: number;
        max_total_entity_pps: number;
    }

export interface EntityServerSettings {
        NoBackup: boolean;
        NoPersist: boolean;
        backupDirectoryPath: string;
        clockSkew: string;
        debugReceiving: boolean;
        debugSending: boolean;
        debugTimestampNow: boolean;
        dynamicDomainVerificationTimeMax: string;
        dynamicDomainVerificationTimeMin: string;
        entityScriptSourceWhitelist: string;
        maxTmpLifetime: string;
        persistFileDownload: boolean;
        persistFilePath: string;
        persistInterval: string;
        statusHost: string;
        statusPort: string;
        verboseDebug: boolean;
        wantEditLogging: boolean;
        wantTerseEditLogging: boolean;
    }

export interface MessagesMixer {
        max_node_messages_per_second: number;
    }

export interface Metaverse {
        access_token: string;
        automatic_networking: string;
        enable_metadata_exporter: boolean;
        enable_packet_verification: boolean;
        id: string;
        local_port: string;
        metadata_exporter_port: string;
    }

export interface Monitoring {
        enable_prometheus_exporter: boolean;
        prometheus_exporter_port: string;
    }

export interface Oauth {
        "admin-roles": string;
        "admin-users": string;
        cert: string;
        "cert-fingerprint": string;
        "client-id": string;
        enable: boolean;
        hostname: string;
        key: string;
        provider: string;
    }

export interface GroupForbidden {
        id_can_adjust_locks: boolean;
        id_can_connect: boolean;
        id_can_connect_past_max_capacity: boolean;
        id_can_get_and_set_private_user_data: boolean;
        id_can_kick: boolean;
        id_can_replace_content: boolean;
        id_can_rez: boolean;
        id_can_rez_avatar_entities: boolean;
        id_can_rez_certified: boolean;
        id_can_rez_tmp: boolean;
        id_can_rez_tmp_certified: boolean;
        id_can_write_to_asset_server: boolean;
        permissions_id: string;
        // rank_id?: string;
        // rank_name?: string;
        // rank_order?: string;
        // group_id?: string;
    }

export interface GroupPermission {
        id_can_adjust_locks: boolean;
        id_can_connect: boolean;
        id_can_connect_past_max_capacity: boolean;
        id_can_get_and_set_private_user_data: boolean;
        id_can_kick: boolean;
        id_can_replace_content: boolean;
        id_can_rez: boolean;
        id_can_rez_avatar_entities: boolean;
        id_can_rez_certified: boolean;
        id_can_rez_tmp: boolean;
        id_can_rez_tmp_certified: boolean;
        id_can_write_to_asset_server: boolean;
        permissions_id: string;
        // rank_id?: string;
        // rank_name?: string;
        // rank_order?: string;
        // group_id?: string;
    }

export interface IpPermission {
        id_can_adjust_locks: boolean;
        id_can_connect: boolean;
        id_can_connect_past_max_capacity: boolean;
        id_can_get_and_set_private_user_data: boolean;
        id_can_kick: boolean;
        id_can_replace_content: boolean;
        id_can_rez: boolean;
        id_can_rez_avatar_entities: boolean;
        id_can_rez_certified: boolean;
        id_can_rez_tmp: boolean;
        id_can_rez_tmp_certified: boolean;
        id_can_write_to_asset_server: boolean;
        permissions_id: string;
    }

export interface MacPermission {
        id_can_adjust_locks: boolean;
        id_can_connect: boolean;
        id_can_connect_past_max_capacity: boolean;
        id_can_get_and_set_private_user_data: boolean;
        id_can_kick: boolean;
        id_can_replace_content: boolean;
        id_can_rez: boolean;
        id_can_rez_avatar_entities: boolean;
        id_can_rez_certified: boolean;
        id_can_rez_tmp: boolean;
        id_can_rez_tmp_certified: boolean;
        id_can_write_to_asset_server: boolean;
        permissions_id: string;
    }

export interface MachineFingerprintPermission {
        id_can_adjust_locks: boolean;
        id_can_connect: boolean;
        id_can_connect_past_max_capacity: boolean;
        id_can_get_and_set_private_user_data: boolean;
        id_can_kick: boolean;
        id_can_replace_content: boolean;
        id_can_rez: boolean;
        id_can_rez_avatar_entities: boolean;
        id_can_rez_certified: boolean;
        id_can_rez_tmp: boolean;
        id_can_rez_tmp_certified: boolean;
        id_can_write_to_asset_server: boolean;
        permissions_id: string;
    }

export interface Permission {
        id_can_adjust_locks: boolean;
        id_can_connect: boolean;
        id_can_connect_past_max_capacity: boolean;
        id_can_get_and_set_private_user_data: boolean;
        id_can_kick: boolean;
        id_can_replace_content: boolean;
        id_can_rez: boolean;
        id_can_rez_avatar_entities: boolean;
        id_can_rez_certified: boolean;
        id_can_rez_tmp: boolean;
        id_can_rez_tmp_certified: boolean;
        id_can_write_to_asset_server: boolean;
        permissions_id: string;
    }

export interface StandardPermission {
        id_can_adjust_locks: boolean;
        id_can_connect: boolean;
        id_can_connect_past_max_capacity: boolean;
        id_can_get_and_set_private_user_data: boolean;
        id_can_kick: boolean;
        id_can_replace_content: boolean;
        id_can_rez: boolean;
        id_can_rez_avatar_entities: boolean;
        id_can_rez_certified: boolean;
        id_can_rez_tmp: boolean;
        id_can_rez_tmp_certified: boolean;
        id_can_write_to_asset_server: boolean;
        permissions_id: string;
    }

export interface Security {
        ac_subnet_whitelist: string[];
        approved_safe_urls: string;
        group_forbiddens: GroupForbidden[];
        group_permissions: GroupPermission[];
        http_username: string;
        ip_permissions: IpPermission[];
        mac_permissions: MacPermission[];
        machine_fingerprint_permissions: any[];
        maximum_user_capacity: string;
        maximum_user_capacity_redirect_location: string;
        multi_kick_logged_in: boolean;
        permissions: Permission[];
        standard_permissions: StandardPermission[];
    }

export interface Webrtc {
        enable_webrtc: boolean;
        enable_webrtc_websocket_ssl: boolean;
    }

export interface Wizard {
        cloud_domain: boolean;
        completed_once: boolean;
        steps_completed: number;
    }

export interface SettingsValues {
        acme?: Acme;
        asset_server?: AssetServer;
        audio_buffer?: AudioBuffer;
        audio_env?: AudioEnv;
        audio_threading?: AudioThreading;
        authentication?: Authentication;
        automatic_content_archives?: AutomaticContentArchives;
        avatar_mixer?: AvatarMixer;
        avatars?: Avatars;
        broadcasting?: Broadcasting;
        descriptors?: Descriptors;
        domain_server?: DomainServer;
        entity_script_server?: EntityScriptServer;
        entity_server_settings?: EntityServerSettings;
        messages_mixer?: MessagesMixer;
        metaverse?: Metaverse;
        monitoring?: Monitoring;
        oauth?: Oauth;
        security?: Security;
        version?: number;
        webrtc?: Webrtc;
        wizard?: Wizard;
    }

export interface SettingsResponse {
        descriptions: Description[];
        values: SettingsValues;
    }

export interface Domains {
        domainId: number,
        id: number,
        name: string
}

export interface DomainsData {
        domains: Domains[]
}

export interface DomainsResponse {
        status: string,
        data: DomainsData,
        current_page: number,
        per_page: number,
        total_pages: number,
        total_entries: number
    }

export interface MetaverseSaveSettings {
    "metaverse": {
        "access_token"?: string,
        "automatic_networking"?: string,
        "enable_metadata_exporter"?: boolean,
        "enable_packet_verification"?: boolean,
        "id"?: string,
        "local_port"?: string,
        "metadata_exporter_port"?: string
    }
}

export interface WebrtcSaveSettings {
    "webrtc": {
        "enable_webrtc": boolean,
        "enable_webrtc_websocket_ssl": boolean
    }
}

export interface WordpressSaveSettings {
    "authentication": {
        "enable_oauth2": boolean,
        "oauth2_url_path": string,
        "plugin_client_id": string,
        "wordpress_url_base": string
    }
}

export interface SSLClientAcmeSaveSettings {
    "acme": {
        "account_key_path"?: string,
        "certificate_authority_filename"?: string,
        "certificate_directory"?: string,
        "certificate_domains"?: string,
        "certificate_filename"?: string,
        "certificate_key_filename"?: string,
        "challenge_handler_type"?: string,
        "directory_endpoint"?: string,
        "eab_kid"?: string,
        "eab_mac"?: string,
        "enable_client"?: boolean,
        "zerossl_api_key"?: string,
        "zerossl_rest_api"?: boolean
    }
}

export interface MonitoringSaveSettings {
    "monitoring": {
        "enable_prometheus_exporter"?: boolean,
        "prometheus_exporter_port"?: string
    }
}

export interface SecuritySaveSettings {
    "security": {
        "ac_subnet_whitelist"?: string[],
        "approved_safe_urls"?: string,
        "group_forbiddens"?: GroupForbidden[],
        "group_permissions"?: GroupPermission[],
        "http_username"?: string,
        "ip_permissions"?: IpPermission[],
        "mac_permissions"?: MacPermission[],
        "machine_fingerprint_permissions"?: any[],
        "maximum_user_capacity"?: string,
        "maximum_user_capacity_redirect_location"?: string,
        "multi_kick_logged_in"?: boolean,
        "permissions"?: Permission[],
        "standard_permissions"?: StandardPermission[]
    }
}

export interface AudioThreadingSaveSettings {
    "audio_threading": {
        "auto_threads"?: boolean;
        "num_threads"?: string;
        "throttle_backoff"?: number;
        "throttle_start"?: number;
    }
}

export interface AudioEnvSaveSettings {
    "audio_env": {
        "codec_preference_order"?: string;
        "enable_filter"?: boolean;
        "noise_muting_threshold"?: string;
    }
}

export interface AudioBufferSaveSettings {
    "audio_buffer": {
        "dynamic_jitter_buffer"?: boolean;
        "max_frames_over_desired"?: string;
        "repetition_with_fade"?: string;
        "static_desired_jitter_buffer_frames"?: string;
        "use_stdev_for_desired_calc"?: string;
        "window_seconds_for_desired_calc_on_too_many_starves"?: string;
        "window_seconds_for_desired_reduction"?: string;
        "window_starve_threshold"?: string;
    }
}

export interface AvatarsSaveSettings {
    "avatars": {
        "avatar_whitelist"?: string;
        "max_avatar_height"?: number;
        "min_avatar_height"?: number;
        "replacement_avatar"?: string;
    }
}

export interface AvatarMixerSaveSettings {
    "avatar_mixer": {
        "auto_threads"?: boolean;
        "connection_rate"?: string;
        "max_node_send_bandwidth"?: number;
        "num_threads"?: string;
        "priority_fraction"?: string;
    }
}
