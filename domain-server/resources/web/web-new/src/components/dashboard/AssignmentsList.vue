<template>
    <div class="q-pa-md">
        <q-table
        title="Queued Assignment Clients"
        :rows="queuedList"
        :columns="queuedColumns"
        row-key="name"
        :rows-per-page-options="[0]"
        :pagination="pagination"
        hide-bottom
        >
        </q-table>
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { Assignments } from "@Modules/domain/assignments";
import type { Assignment, QueuedAssignment } from "@Modules/domain/interfaces/assignments";

const queuedColumns = [
    { name: "type", align: "left", label: "Type", field: "type" },
    { name: "id", align: "left", label: "UUID", field: "id" }
];

export default defineComponent({
    name: "AssignmentsList",

    data () {
        return {
            queuedColumns,

            pagination: {
                page: 1,
                rowsPerPage: 0 // 0 means all rows
            },
            fulfilledList: [] as Assignment[],
            queuedList: [] as QueuedAssignment[],
            timer: 0 as number
        };
    },

    methods: {
        async refreshAssignmentsList (): Promise<void> {
            const assignmentsResult = await Assignments.getAllAssignments();

            this.fulfilledList = assignmentsResult.fulfilled;
            this.queuedList = assignmentsResult.queued;
        },
        cancelAutoUpdate () {
            clearInterval(this.timer);
        }
    },

    mounted () {
        this.refreshAssignmentsList();
        // eslint-disable-next-line @typescript-eslint/no-misused-promises
        this.timer = window.setInterval(this.refreshAssignmentsList, 2000);
    },

    beforeUnmount () {
        this.cancelAutoUpdate();
    }
});
</script>
