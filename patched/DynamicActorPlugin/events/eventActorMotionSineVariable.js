const l10n = require("../helpers/l10n").default;
const {
  precompileScriptValue,
  optimiseScriptValue,
} = require("shared/lib/scriptValue/helpers");

export const id = "EVENT_ACTOR_MOTION_SINE_VAR";
export const name = "Actor Motion: Sine Wave (Variable)";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = (fetchArg, input) => {
  return `Sine Wave (Variable) : ${fetchArg("actorId")}`;
};

export const fields = [
  {
    key: "actorId",
    label: l10n("ACTOR"),
    description:
      "Actor to move. Needs a behavior with Move X / Move Y enabled. Unlike the plain Sine Wave event, amplitude/period/duration accept variables and expressions, evaluated when the event starts (uses the engine's sine table at runtime).",
    type: "actor",
    defaultValue: "$self$",
  },
  {
    key: "axis",
    label: "Axis",
    description: "Axis to oscillate on. The other axis is left untouched.",
    type: "select",
    options: [
      ["y", "Vertical (Y)"],
      ["x", "Horizontal (X)"],
    ],
    defaultValue: "y",
  },
  {
    key: "amplitude",
    label: "Amplitude (px)",
    description:
      "Peak distance from the start position in pixels (1-160). The wave speed is capped at ±127 subpixels/frame at runtime.",
    type: "value",
    min: 1,
    max: 160,
    defaultValue: {
      type: "number",
      value: 16,
    },
  },
  {
    key: "period",
    label: "Period (frames)",
    description: "Frames for one full wave cycle (60 = 1 second, min 16)",
    type: "value",
    min: 16,
    max: 1024,
    defaultValue: {
      type: "number",
      value: 120,
    },
  },
  {
    key: "duration",
    label: "Duration (frames, 0 = forever)",
    description: "How long to keep waving before continuing",
    type: "value",
    min: 0,
    max: 32767,
    defaultValue: {
      type: "number",
      value: 0,
    },
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
    key: "invert",
    label: "Invert (start moving up / left)",
    description: "Start the wave in the negative direction",
    type: "checkbox",
    defaultValue: false,
  },
  {
    key: "stopAtEnd",
    label: "Stop at end",
    description: "Zero the axis velocity when the duration ends",
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
  const axis = input.axis === "x" ? "x" : "y";
  const setNative =
    axis === "x" ? "vm_set_actor_velocity_x" : "vm_set_actor_velocity_y";

  const actorRef = _declareLocal("tmp0", 1, true);
  const vmaxRef = _declareLocal("sinv_vmax", 1, true);
  const perRef = _declareLocal("sinv_per", 1, true);
  const stepRef = _declareLocal("sinv_step", 1, true);
  const angleRef = _declareLocal("sinv_angle", 1, true);
  const velRef = _declareLocal("sinv_vel", 1, true);
  const durRef = _declareLocal("sinv_dur", 1, true);
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

  _addComment(`Actor Motion: Sine Wave Variable (${axis})`);

  evalToLocal(input.amplitude, vmaxRef, 16);
  evalToLocal(input.period, perRef, 120);
  evalToLocal(input.duration, durRef, 0);

  // per = max(per, 16); vmax = clamp(amp,0,160)*201/per capped at 127
  // (201 = 2*pi*32: peak velocity of a wave in subpixels/frame)
  _rpn().ref(perRef).int16(16).operator(".MAX").refSet(perRef).stop();
  const setupRpn = _rpn()
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
    .operator(".MIN");
  if (input.invert === true) {
    setupRpn.operator(".NEG");
  }
  setupRpn.refSet(vmaxRef).stop();
  // angle step per update, 256 = full cycle
  _rpn()
    .int16(256 * interval)
    .ref(perRef)
    .operator(".DIV")
    .int16(1)
    .operator(".MAX")
    .refSet(stepRef)
    .stop();
  _setConst(angleRef, 0);

  const topLabel = getNextLabel();
  const doneLabel = getNextLabel();
  _label(topLabel);
  // vel = vmax * sin(angle)
  _rpn().ref(vmaxRef).refSet(velRef).stop();
  appendRaw(`VM_SIN_SCALE ${velRef}, ${angleRef}, 7`);
  _stackPush(velRef);
  _stackPush(actorRef);
  _callNative(setNative);
  _stackPop(2);
  // advance the angle (wraps at 256)
  _rpn()
    .ref(angleRef)
    .ref(stepRef)
    .operator(".ADD")
    .int16(255)
    .operator(".B_AND")
    .refSet(angleRef)
    .stop();
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
  _label(doneLabel);

  if (input.stopAtEnd !== false) {
    _stackPushConst(0);
    _stackPush(actorRef);
    _callNative(setNative);
    _stackPop(2);
  }

  _addNL();
};
