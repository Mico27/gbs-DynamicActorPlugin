export const id = "EVENT_WAIT_FOR_ACTOR_STATE_BY_INDEX";
export const name = "Wait For Actor State By Index";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = (fetchArg, input) => {
  return `Wait For Actor State : ${fetchArg("actorIndex")}`;
};

export const fields = [
  {
    key: "actorIndex",
    label: "Actor Index",
    description: "Index of the actor whose behavior state to wait for. Grounded/airborne are managed by behaviors with Gravity + Move Y enabled.",
    type: "value",
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
  {
    key: "state",
    label: "Wait until state is",
    description: "Behavior state to wait for",
    type: "select",
    options: [
      [1, "Grounded (landed)"],
      [2, "Airborne Y (in the air by Y movement)"],
      [3, "Airborne Z (pos_z above ground)"],
      [0, "Paused"],
    ],
    defaultValue: 1,
  },
  {
    key: "invert",
    label: "Invert (wait until NOT in this state)",
    description: "Continue once the actor leaves the chosen state instead",
    type: "checkbox",
    defaultValue: false,
  },
];

export const compile = (input, helpers) => {
  const {
    variableSetToScriptValue,
    _callNative,
    _stackPush,
    _stackPushConst,
    _stackPop,
    _addComment,
    _addNL,
    _declareLocal,
    _idle,
    _label,
    _jump,
    _ifConst,
    getNextLabel,
  } = helpers;

  const state = [0, 1, 2, 3].includes(Number(input.state))
    ? Number(input.state)
    : 1;
  const invert = input.invert === true;

  const actorRef = _declareLocal("tmp0", 1, true);
  const stateRef = _declareLocal("bhv_state", 1, true);

  variableSetToScriptValue(actorRef, input.actorIndex);

  _addComment(`Wait For Actor State ${invert ? "!=" : "=="} ${state}`);

  const loopLabel = getNextLabel();
  const doneLabel = getNextLabel();
  _label(loopLabel);
  _stackPushConst(stateRef);
  _stackPush(actorRef);
  _callNative("vm_get_actor_state");
  _stackPop(2);
  _ifConst(invert ? ".NE" : ".EQ", stateRef, state, doneLabel, 0);
  _idle();
  _jump(loopLabel);
  _label(doneLabel);

  _addNL();
};
