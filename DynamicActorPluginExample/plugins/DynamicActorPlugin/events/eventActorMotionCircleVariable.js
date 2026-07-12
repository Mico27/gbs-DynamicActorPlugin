const l10n = require("../helpers/l10n").default;
const {
  precompileScriptValue,
  optimiseScriptValue,
} = require("shared/lib/scriptValue/helpers");

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
      "Actor to move. Needs a behavior with Move X + Move Y enabled (usually with tile collision off). Unlike the plain Circle event, radius/period/duration accept variables and expressions, evaluated when the event starts (uses the engine's sine table at runtime).",
    type: "actor",
    defaultValue: "$self$",
  },
  {
    key: "radius",
    label: "Radius (px)",
    description:
      "Circle radius in pixels (1-160). The orbit speed is capped at ±127 subpixels/frame at runtime.",
    type: "value",
    min: 1,
    max: 160,
    defaultValue: {
      type: "number",
      value: 24,
    },
  },
  {
    key: "period",
    label: "Period (frames)",
    description: "Frames for one full revolution (60 = 1 second, min 16)",
    type: "value",
    min: 16,
    max: 1024,
    defaultValue: {
      type: "number",
      value: 180,
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
  {
    key: "stopAtEnd",
    label: "Stop at end",
    description: "Zero the velocity when the duration ends",
    type: "checkbox",
    defaultValue: true,
  },
];

export const compile = (input, helpers) => {
  const {
    _callNative,
    _stackPush,
    _stackPushConst,
    _stackPop,
    _addComment,
    _addNL,
    _declareLocal,
    setActorId,
    _setConst,
    _invoke,
    _idle,
    _label,
    _jump,
    _ifConst,
    _rpn,
    _performFetchOperations,
    _performValueRPN,
    appendRaw,
    getNextLabel,
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
  const vmaxRef = _declareLocal("circv_vmax", 1, true);
  const perRef = _declareLocal("circv_per", 1, true);
  const stepRef = _declareLocal("circv_step", 1, true);
  const angleRef = _declareLocal("circv_angle", 1, true);
  const velXRef = _declareLocal("circv_velx", 1, true);
  const velYRef = _declareLocal("circv_vely", 1, true);
  const durRef = _declareLocal("circv_dur", 1, true);
  const waitArgsRef = _declareLocal("wait_args", 1, true);

  const evalToLocal = (value, localRef, fallback) => {
    const [rpnOps, fetchOps] = precompileScriptValue(
      optimiseScriptValue(
        value != null ? value : { type: "number", value: fallback },
      ),
    );
    const localsLookup = _performFetchOperations(fetchOps);
    const rpn = _rpn();
    _performValueRPN(rpn, rpnOps, localsLookup);
    rpn.refSet(localRef).stop();
  };

  const waitN = (frames) => {
    if (frames <= 0) return;
    if (frames < 5) {
      for (let i = 0; i < frames; i++) _idle();
    } else {
      _setConst(waitArgsRef, frames);
      _invoke("wait_frames", 0, waitArgsRef);
    }
  };

  setActorId(actorRef, input.actorId);

  _addComment(`Actor Motion: Circle Variable (${ccw ? "ccw" : "cw"})`);

  evalToLocal(input.radius, vmaxRef, 24);
  evalToLocal(input.period, perRef, 180);
  evalToLocal(input.duration, durRef, 0);

  // per = max(per, 16); vmax = clamp(radius,0,160)*201/per capped at 127
  // (201 = 2*pi*32: orbit speed in subpixels/frame)
  _rpn().ref(perRef).int16(16).operator(".MAX").refSet(perRef).stop();
  _rpn()
    .ref(vmaxRef)
    .int16(0)
    .operator(".MAX")
    .int16(160)
    .operator(".MIN")
    .int16(201)
    .operator(".MUL")
    .ref(perRef)
    .operator(".DIV")
    .int16(127)
    .operator(".MIN")
    .refSet(vmaxRef)
    .stop();
  // angle step per update, 256 = full revolution
  _rpn()
    .int16(256 * interval)
    .ref(perRef)
    .operator(".DIV")
    .int16(1)
    .operator(".MAX")
    .refSet(stepRef)
    .stop();
  _setConst(angleRef, startAngleUnits);

  const topLabel = getNextLabel();
  _label(topLabel);
  // vel = (vmax*sin(angle), -vmax*cos(angle)) — angle 0 = top of circle
  _rpn().ref(vmaxRef).refSet(velXRef).stop();
  appendRaw(`VM_SIN_SCALE ${velXRef}, ${angleRef}, 7`);
  _rpn().ref(vmaxRef).refSet(velYRef).stop();
  appendRaw(`VM_COS_SCALE ${velYRef}, ${angleRef}, 7`);
  _rpn().ref(velYRef).operator(".NEG").refSet(velYRef).stop();
  _stackPush(velYRef);
  _stackPush(velXRef);
  _stackPush(actorRef);
  _callNative("vm_set_actor_velocity");
  _stackPop(3);
  // advance the angle (wraps at 256)
  const angleRpn = _rpn().ref(angleRef).ref(stepRef);
  angleRpn.operator(ccw ? ".SUB" : ".ADD");
  angleRpn.int16(255).operator(".B_AND").refSet(angleRef).stop();
  waitN(interval);
  // duration: 0 = forever, otherwise count down and stop at 0
  _ifConst(".EQ", durRef, 0, topLabel, 0);
  _rpn()
    .ref(durRef)
    .int16(interval)
    .operator(".SUB")
    .int16(0)
    .operator(".MAX")
    .refSet(durRef)
    .stop();
  _ifConst(".GT", durRef, 0, topLabel, 0);

  if (input.stopAtEnd !== false) {
    _stackPushConst(0);
    _stackPushConst(0);
    _stackPush(actorRef);
    _callNative("vm_set_actor_velocity");
    _stackPop(3);
  }

  _addNL();
};
