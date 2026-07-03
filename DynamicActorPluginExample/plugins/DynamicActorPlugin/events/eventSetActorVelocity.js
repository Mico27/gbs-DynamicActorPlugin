const l10n = require("../helpers/l10n").default;

export const id = "EVENT_SET_ACTOR_VELOCITY";
export const name = "Set Actor Velocity";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = (fetchArg) => {
  return `Set Actor Velocity : ${fetchArg("xVelocity")}, ${fetchArg("yVelocity")}`;
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
    key: "xVelocity",
    label: "X velocity",
    description:
      "Horizontal velocity in subpixels per frame (16 = 1 pixel/frame, negative = left)",
    type: "value",
    min: -32768,
    max: 32767,
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
  {
    key: "yVelocity",
    label: "Y velocity",
    description:
      "Vertical velocity in subpixels per frame (16 = 1 pixel/frame, negative = up)",
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

  setActorId(tmp0, input.actorId);
  variableSetToScriptValue(tmp1, input.xVelocity);
  variableSetToScriptValue(tmp2, input.yVelocity);

  _addComment("Set Actor Velocity");

  _stackPush(tmp2);
  _stackPush(tmp1);
  _stackPush(tmp0);

  _callNative("vm_set_actor_velocity");
  _stackPop(3);
};
