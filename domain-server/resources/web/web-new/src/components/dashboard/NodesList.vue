<template>
    <div class="q-pa-md">
        <q-dialog v-model="confirmKillAll" persistent>
            <q-card>
                <q-card-section class="row items-center">
                    <q-avatar icon="mdi-alert" text-color="warning" size="80px"/>
                    <span class="q-ml-sm">Are you sure you want to kill all nodes?</span>
                </q-card-section>

                <q-card-actions align="right">
                    <q-btn flat label="Cancel" color="primary" v-close-popup />
                    <q-btn flat label="Yes - Kill all" color="primary" @click="killAllNodes" v-close-popup />
                </q-card-actions>
            </q-card>
        </q-dialog>

        <q-table
            title="Nodes"
            :rows="nodesList"
            :columns="columns"
            row-key="name"
            :rows-per-page-options="[0]"
            :pagination="pagination"
        >
            <template v-slot:body="props">
                <q-tr :props="props">
                    <q-td key="type" :props="props">
                        {{ props.row.type }}
                    </q-td>
                    <q-td key="version" :props="props">
                        {{ props.row.version }}
                    </q-td>
                    <q-td key="uuid" :props="props">
                        {{ props.row.uuid }}
                    </q-td>
                    <q-td key="username" :props="props">
                        {{ props.row.username }}
                    </q-td>
                    <q-td key="public" :props="props">
                        {{ props.row.public.ip }} : {{ props.row.public.port }}
                    </q-td>
                    <q-td key="local" :props="props">
                        {{ props.row.local.ip }} : {{ props.row.local.port }}
                    </q-td>
                    <q-td key="uptime" :props="props">
                        {{ props.row.uptime }}
                    </q-td>
                    <q-td key="uuid" :props="props">
                        <q-btn color="red" @click="killNode(props.row.uuid)" icon="mdi-close-box" dense/>
                    </q-td>
                </q-tr>
            </template>

            <template v-slot:bottom>
                <q-btn color="red" label="Kill all Nodes" @click="confirmKillAll = true" icon="mdi-close-box-multiple" />
            </template>
        </q-table>
    </div>
</template>

<script lang="ts">
import { defineComponent, ref } from "vue";
import { Nodes } from "@Modules/domain/nodes";
import type { Node } from "@Modules/domain/interfaces/nodes";

const columns = [
    { name: "type", align: "left", label: "Type", field: "type" },
    { name: "version", align: "left", label: "Version", field: "version" },
    {
        name: "uuid",
        align: "left",
        label: "UUID",
        field: "uuid",
        style: "max-width: 200px",
        headerStyle: "max-width: 200px"
    },
    { name: "username", align: "left", label: "Username", field: "username" },
    { name: "public", align: "left", label: "Public", field: "public" },
    { name: "local", align: "left", label: "Local", field: "local.ip" },
    { name: "uptime", align: "left", label: "Uptime (s)", field: "uptime" },
    { name: "kill", align: "left", label: "Kill ?", field: "uuid" }
];

export default defineComponent({
    name: "NodesList",
    data () {
        return {
            columns,
            pagination: {
                page: 1,
                rowsPerPage: 0 // 0 means all rows.
            },
            nodesList: [] as Node[],
            timer: 0 as number,
            confirmKillAll: ref(false)
        };
    },
    methods: {
        async refreshNodesList (): Promise<void> {
            const nodesResult = await Nodes.getNodes();
            this.nodesList = nodesResult;
        },
        cancelAutoUpdate (): void {
            clearInterval(this.timer);
        },
        async killNode (nodeUuid: string): Promise<void> {
            await Nodes.killNode(nodeUuid);
        },
        async killAllNodes (): Promise<void> {
            // TODO: add a confirmation question here.
            await Nodes.killAllNodes();
        }
    },
    mounted () {
        this.refreshNodesList();
        // eslint-disable-next-line @typescript-eslint/no-misused-promises
        this.timer = window.setInterval(this.refreshNodesList, 2000);
    },
    beforeUnmount () {
        this.cancelAutoUpdate();
    }
});
</script>
