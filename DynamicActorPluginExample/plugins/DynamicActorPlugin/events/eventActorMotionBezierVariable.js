const l10n = require("../helpers/l10n").default;

export const id = "EVENT_ACTOR_MOTION_BEZIER_VAR";
export const name = "Actor Motion: Bezier (Variable)";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = (fetchArg) => {
  return `Bezier Motion (Variable) : ${fetchArg("actorId")}`;
};

const scriptNumber = (value, fallback = 0) => {
  if (value && typeof value === "object") {
    return value;
  }
  const n = Number(value);
  return {
    type: "number",
    value: Number.isFinite(n) ? n : fallback,
  };
};

const packXY = (x, y) => ({
  type: "add",
  valueA: scriptNumber(x, 0),
  valueB: {
    type: "shl",
    valueA: scriptNumber(y, 0),
    valueB: {
      type: "number",
      value: 8,
    },
  },
});

const packIncrementalLerp = (incremental) => ({
  type: "add",
  valueA: {
    type: "shl",
    valueA: scriptNumber(incremental, 8),
    valueB: {
      type: "number",
      value: 8,
    },
  },
  valueB: {
    type: "number",
    value: 0,
  },
});

export const fields = [
  {
    key: "actorId",
    label: l10n("ACTOR"),
    description: "Actor to move along the Bezier path",
    type: "actor",
    defaultValue: "$self$",
  },
  {
    key: "bezierType",
    label: "Curve type",
    description: "Use quadratic (3 points) or cubic (4 points)",
    type: "select",
    options: [
      ["quadratic", "Quadratic"],
      ["cubic", "Cubic"],
    ],
    defaultValue: "quadratic",
  },
  {
    key: "incremental",
    label: "Increment per update",
    description:
      "Lerp step in 0..255 units per update. Larger values finish faster.",
    type: "value",
    min: 1,
    max: 255,
    defaultValue: {
      type: "number",
      value: 8,
    },
  },
  {
    key: "p0x",
    label: "P0 X (px)",
    type: "value",
    min: -128,
    max: 127,
    width: "50%",
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
  {
    key: "p0y",
    label: "P0 Y (px)",
    type: "value",
    min: -128,
    max: 127,
    width: "50%",
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
  {
    key: "p1x",
    label: "P1 X (px)",
    type: "value",
    min: -128,
    max: 127,
    width: "50%",
    defaultValue: {
      type: "number",
      value: 32,
    },
  },
  {
    key: "p1y",
    label: "P1 Y (px)",
    type: "value",
    min: -128,
    max: 127,
    width: "50%",
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
  {
    key: "p2x",
    label: "P2 X (px)",
    type: "value",
    min: -128,
    max: 127,
    width: "50%",
    defaultValue: {
      type: "number",
      value: 64,
    },
  },
  {
    key: "p2y",
    label: "P2 Y (px)",
    type: "value",
    min: -128,
    max: 127,
    width: "50%",
    defaultValue: {
      type: "number",
      value: 32,
    },
  },
  {
    key: "p3x",
    label: "P3 X (px)",
    type: "value",
    min: -128,
    max: 127,
    width: "50%",
    defaultValue: {
      type: "number",
      value: 96,
    },
    conditions: [
      {
        key: "bezierType",
        eq: "cubic",
      },
    ],
  },
  {
    key: "p3y",
    label: "P3 Y (px)",
    type: "value",
    min: -128,
    max: 127,
    width: "50%",
    defaultValue: {
      type: "number",
      value: 0,
    },
    conditions: [
      {
        key: "bezierType",
        eq: "cubic",
      },
    ],
  },
];

export const compile = (input, helpers) => {
  const {
    _stackPush,
    _stackPushConst,
    _stackPushScriptValue,
    _addComment,
    _addNL,
    _declareLocal,
    setActorId,
    _invoke,
  } = helpers;

  const actorRef = _declareLocal("actorRef", 1, true);
  setActorId(actorRef, input.actorId);

  const isCubic = input.bezierType === "cubic";

  _addComment(`Actor Motion: Bezier Variable (${isCubic ? "cubic" : "quadratic"})`);

  _stackPush(actorRef);
  _stackPushConst(0);
  _stackPushConst(0);
  _stackPushConst(isCubic ? 1 : 0);
  _stackPushScriptValue(packIncrementalLerp(input.incremental));
  _stackPushScriptValue(packXY(input.p0x, input.p0y));
  _stackPushScriptValue(packXY(input.p1x, input.p1y));
  _stackPushScriptValue(packXY(input.p2x, input.p2y));
  _stackPushScriptValue(isCubic ? packXY(input.p3x, input.p3y) : scriptNumber(0));
  _stackPushScriptValue(scriptNumber(input.p0x, 0));
  _stackPushScriptValue(scriptNumber(input.p0y, 0));

  _invoke("vm_actor_move_bezier_to", 11, ".ARG10");
  _addNL();
};
