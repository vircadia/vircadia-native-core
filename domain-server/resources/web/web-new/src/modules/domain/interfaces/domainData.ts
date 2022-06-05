/* eslint-disable @typescript-eslint/no-explicit-any */
/* eslint-disable camelcase */
export interface Meta {
        images: any[];
        managers: any[];
        tags: any[];
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
        owner_places: any[];
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
        maturity: string;
        managers: any[];
        tags: any[];
        meta: Meta;
        users: Users;
        time_of_last_heartbeat: Date;
        time_of_last_heartbeat_s: string;
        addr_of_first_contact: string;
        when_domain_entry_created: Date;
        when_domain_entry_created_s: string;
    }

export interface Data {
        domain: Domain;
    }

export interface DomainData {
        status: string;
        data: Data;
        domain: Domain;
    }
