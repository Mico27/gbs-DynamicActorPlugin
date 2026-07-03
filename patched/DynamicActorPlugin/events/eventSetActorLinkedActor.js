const l10n = require("../helpers/l10n").default;

export const id = "EVENT_SET_ACTOR_LINKED_ACTOR";
export const name = "Set Actor Linked Actor";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = (fetchArg) => {
  return `Set Actor Linked Actor`;
};

export const fields = [
  {
    key: "actorId",
    label: l10n("ACTOR"),
    description: "Actor that will follow the linked actor",
    type: "actor",
    defaultValue: "$self$",
  },
  {
    key: "linkedActorId",
    label: "Linked actor",
    description: "Actor to follow (used by behaviors with 'Attach to linked actor')",
    type: "actor",
    defaultValue: "$self$",
  },
  {
    key: "offsetX",
    label: "X offset",
    description: "Horizontal offset from the linked actor in subpixels (16 = 1 pixel)",
    type: "value",
    min: -32768,
    max: 32767,
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
  {
    key: "offsetY",
    label: "Y offset",
    description: "Vertical offset from the linked actor in subpixels (16 = 1 pixel)",
    type: "value",
    min: -32768,
    max: 32767,
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
];

export const compile = (input, helpers) => {
  const {
    _callNative,
    _stackPush,
    _stackPop,
    _addComment,
    _declareLocal,
    variableSetToScriptValue,
    setActorId,
  } = helpers;

  const tmp0 = _declareLocal("tmp0", 1, true);
  const tmp1 = _declareLocal("tmp1", 1, true);
  const tmp2 = _declareLocal("tmp2", 1, true);
  const tmp3 = _declareLocal("tmp3", 1, true);

  setActorId(tmp0, input.actorId);
  setActorId(tmp1, input.linkedActorId);
  variableSetToScriptValue(tmp2, input.offsetX || { type: "number", value: 0 });
  variableSetToScriptValue(tmp3, input.offsetY || { type: "number", value: 0 });

  _addComment("Set Actor Linked Actor");

  _stackPush(tmp3);
  _stackPush(tmp2);
  _stackPush(tmp1);
  _stackPush(tmp0);

  _callNative("vm_set_actor_linked_actor_idx");
  _stackPop(4);
};
