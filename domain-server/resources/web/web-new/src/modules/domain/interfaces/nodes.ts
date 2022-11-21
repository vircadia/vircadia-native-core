export interface IpAddress {
    ip: string;
    port: number;
}

export interface Node {
    local: IpAddress;
    pool: string;
    public: IpAddress;
    type: string;
    uptime: string;
    username: string;
    uuid: string;
    version: string;
}

export interface GetNodesResp {
    "nodes": Node[],
}
