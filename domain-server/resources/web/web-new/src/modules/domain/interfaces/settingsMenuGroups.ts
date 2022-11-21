
export interface ContentSetting {
    "html_id": string;
    label: string;
    name: string;
}

export interface DomainSetting {
    label: string;
    name: string;
}

export interface SettingsMenuGroups {
    "content_settings": ContentSetting[];
    "domainsettings": DomainSetting[];
}
