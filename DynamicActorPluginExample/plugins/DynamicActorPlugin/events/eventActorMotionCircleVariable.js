const l10n = require("../helpers/l10n").default;

export const id = "EVENT_ACTOR_MOTION_CIRCLE_VAR";
export const name = "Actor Motion: Circle (Variable)";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = (fetchArg, input) => {
  return `Circle (Variable) : ${fetchArg("actorId")}`;
};

export const fields = [
  {
    key: "actorId",
    label: l10n("ACTOR"),
    description:
      "Actor to move. Needs a behavior with Move X + Move Y enabled (usually with tile collision off). Unlike the plain Circle event, radius/duration accept variables and expressions, and the actor orbits at its own movement speed (change it with Actor Set Movement Speed).",
    type: "actor",
    defaultValue: "$self$",
  },
  {
    key: "radius",
    label: "Radius (px)",
    description:
      "Circle radius in pixels (1-160). Revolution time follows from the radius and the actor's movement speed.",
    type: "value",
    min: 1,
    max: 160,
    defaultValue: {
      type: "number",
      value: 24,
    },
  },
  {
    key: "duration",
    label: "Duration (frames, 0 = forever)",
    description: "How long to keep circling before continuing",
    type: "value",
    min: 0,
    max: 32767,
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
  {
    key: "direction",
    label: "Direction",
    description: "Direction of rotation",
    type: "select",
    options: [
      ["cw", "Clockwise"],
      ["ccw", "Counter-clockwise"],
    ],
    defaultValue: "cw",
  },
  {
    key: "startAngle",
    label: "Start angle (degrees)",
    description:
      "Where on the circle the actor starts: 0 = top, 90 = right, 180 = bottom, 270 = left",
    type: "number",
    min: 0,
    max: 359,
    defaultValue: 0,
  },
  {
    key: "updateInterval",
    label: "Update every (frames)",
    description: "Frames between velocity updates",
    type: "number",
    min: 1,
    max: 16,
    defaultValue: 2,
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

  const interval = Math.max(
    1,
    Math.min(16, Math.round(Number(input.updateInterval)) || 2),
  );
  const ccw = input.direction === "ccw";
  let startAngle = Math.round(Number(input.startAngle)) || 0;
  startAngle = Math.max(0, Math.min(359, startAngle));
  const startAngleUnits = Math.round((startAngle * 256) / 360) & 255;

  const actorRef = _declareLocal("tmp0", 1, true);
  const toScriptNumber = (value, fallback) => {
    if (value && typeof value === "object") {
      return value;
    }
    const n = Number(value);
    return { type: "number", value: Number.isFinite(n) ? n : fallback };
  };

  setActorId(actorRef, input.actorId);

  _addComment(`Actor Motion: Circle Variable (${ccw ? "ccw" : "cw"})`);

  _stackPush(actorRef);
  _stackPushScriptValue(toScriptNumber(input.radius, 24));
  _stackPushScriptValue(toScriptNumber(input.duration, 0));
  _stackPushConst(ccw ? 1 : 0);
  _stackPushConst(startAngleUnits);
  _stackPushConst(interval);
  _stackPushConst(0);
  _stackPushConst(0);
  _stackPushConst(0);
  _invoke("vm_actor_motion_circle_variable", 9, ".ARG8");

  _addNL();
};
