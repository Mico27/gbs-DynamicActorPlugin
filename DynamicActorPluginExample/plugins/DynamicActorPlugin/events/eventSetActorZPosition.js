const l10n = require("../helpers/l10n").default;

export const id = "EVENT_SET_ACTOR_Z_POSITION";
export const name = "Set actor z position";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = () => {
  return "Set actor z position";
};

export const fields = [
  {
    key: "actorId",
    label: l10n("ACTOR"),
    description: l10n("FIELD_ACTOR_DEACTIVATE_DESC"),
    type: "actor",
    defaultValue: "$self$",
  },
  {
    key: "zPosition",
    label: "Z position",
    description: "Actor Z position in subpixels (16 = 1 pixel)",
    type: "value",
    min: 0,
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
    setActorId,
    _stackPushScriptValue,
  } = helpers;

  const tmp0 = _declareLocal("tmp0", 1, true);

  setActorId(tmp0, input.actorId);

  _addComment("Set actor z position");

  _stackPushScriptValue(input.zPosition);
  _stackPush(tmp0);

  _callNative("vm_set_actor_z_position");
  _stackPop(2);
};
