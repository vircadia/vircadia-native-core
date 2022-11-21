/* eslint-disable camelcase */

export interface OwnerPlace {
        placeId: string;
        id: string;
        name: string;
        visibility: string;
        address: string;
        path: string;
        description: string;
        maturity: string;
        tags: string[];
        managers: string[];
        thumbnail: string;
        images: string[];
        current_attendance: number;
        last_activity_update: Date;
        last_activity_update_s: string;
    }

export interface Meta {
        capacity: number;
        contact_info: string;
        description: string;
        images: string[];
        managers: string[];
        restriction: string;
        tags: string[];
        thumbnail: string;
        world_name: string;
    }

export interface Users {
        num_anon_users: number;
        num_users: number;
    }

export interface Domain {
        domainId: string;
        id: string;
        name: string;
        visibility: string;
        world_name: string;
        label: string;
        public_key: string;
        owner_places: OwnerPlace[];
        sponsor_account_id: string;
        ice_server_address: string;
        version: string;
        protocol_version: string;
        network_address: string;
        network_port: number;
        automatic_networking: string;
        restricted: boolean;
        num_users: number;
        anon_users: number;
        total_users: number;
        capacity: number;
        description: string;
        maturity: string;
        restriction: string;
        managers: string[];
        tags: string[];
        meta: Meta;
        users: Users;
        time_of_last_heartbeat: Date;
        time_of_last_heartbeat_s: string;
        addr_of_first_contact: string;
        when_domain_entry_created: Date;
        when_domain_entry_created_s: string;
    }

export interface Data {
        domains: Domain[];
    }

export interface DomainsResponse {
        status: string;
        data: Data;
        current_page: number;
        per_page: number;
        total_pages: number;
        total_entries: number;
    }
