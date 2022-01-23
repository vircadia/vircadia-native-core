export interface MainState {
    prop: boolean;
}

function state (): MainState {
    return {
        prop: false
    };
}

export default state;
