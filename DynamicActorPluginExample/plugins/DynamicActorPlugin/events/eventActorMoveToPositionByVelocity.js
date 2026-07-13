const l10n = require("../helpers/l10n").default;

export const id = "EVENT_ACTOR_MOVE_TO_POSITION_BY_VELOCITY";
export const name = "Actor Move To Position By Velocity";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = (fetchArg, input) => {
  const unitPostfix =
    input.units === "pixels" ? l10n("FIELD_PIXELS_SHORT") : "";
  return l10n("EVENT_ACTOR_MOVE_TO_LABEL", {
    actor: fetchArg("actorId"),
    x: `${fetchArg("targetX")}${unitPostfix}`,
    y: `${fetchArg("targetY")}${unitPostfix}`,
  });
};

export const fields = [
  {
    key: "actorId",
    label: l10n("ACTOR"),
    description: "Actor to move",
    type: "actor",
    defaultValue: "$self$",
  },
  {
    key: "targetX",
    label: "Target X",
    description: "Destination X position",
    type: "value",
    width: "50%",
    defaultValue: {
      type: "number",
      value: 0,
    },
    unitsField: "units",
    unitsDefault: "tiles",
    unitsAllowed: ["tiles", "pixels"],
  },
  {
    key: "targetY",
    label: "Target Y",
    description: "Destination Y position",
    type: "value",
    width: "50%",
    defaultValue: {
      type: "number",
      value: 0,
    },
    unitsField: "units",
    unitsDefault: "tiles",
    unitsAllowed: ["tiles", "pixels"],
  },
  {
    key: "stopRange",
    label: "Stop range (px)",
    description:
      "Completes once the actor is within this range of the destination on every steered axis. 0 = exact match.",
    type: "value",
    min: 0,
    max: 32767,
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
];

const shiftLeftScriptValueConst = (value, num) => {
  return {
    type: "shl",
    valueA: value,
    valueB: {
      type: "number",
      value: num,
    },
  };
};

const scriptValueToPixels = (value, units) => {
  if (units === "pixels") {
    return value;
  }
  return shiftLeftScriptValueConst(value, 3);
};

export const compile = (input, helpers) => {
  const {
    _invoke,
    _stackPush,
    _stackPushScriptValue,
    _addComment,
    _declareLocal,
    setActorId,
  } = helpers;

  const actorRef = _declareLocal("actorRef", 1, true);
  setActorId(actorRef, input.actorId);

  _addComment("Actor Move To Position By Velocity");
  _stackPushScriptValue(
    scriptValueToPixels(input.targetX || { type: "number", value: 0 }, input.units),
  );
  _stackPushScriptValue(
    scriptValueToPixels(input.targetY || { type: "number", value: 0 }, input.units),
  );
  _stackPushScriptValue(input.stopRange || { type: "number", value: 0 });

  _invoke("vm_actor_move_to_pos_by_velocity", 4, ".ARG3");
};

