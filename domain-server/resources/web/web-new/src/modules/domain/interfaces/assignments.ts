export interface IpAddress {
    ip: string;
    port: number;
}

export interface Assignment {
    id: string;
    local: IpAddress;
    pool: string;
    public: IpAddress;
    type: string;
    uptime: string;
    username: string;
    uuid: string;
    version: string;
}

export interface QueuedAssignment {
    id: string;
    type: string;
}

export interface GetAssigmentsResp {
    "fulfilled": Record<string, Assignment>,
    "queued": Record<string, QueuedAssignment>
}

export interface GetAllAssigmentsResp {
    "fulfilled": Assignment[],
    "queued": QueuedAssignment[]
}
