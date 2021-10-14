import { GetterTree } from "vuex";
import { StateInterface } from "../index";
import { MainState } from "./state";

const getters: GetterTree<MainState, StateInterface> = {
    someAction (/* context */) {
    // your code
    }
};

export default getters;
