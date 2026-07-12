const l10n = require("../helpers/l10n").default;
const {
  precompileScriptValue,
  optimiseScriptValue,
} = require("shared/lib/scriptValue/helpers");

export const id = "EVENT_ACTOR_MOTION_HOMING";
export const name = "Actor Motion: Home At Target";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = (fetchArg, input) => {
  return `Home At Target : ${fetchArg("actorId")}`;
};

export const fields = [
  {
    key: "actorId",
    label: l10n("ACTOR"),
    description:
      "Actor that homes in. Needs a behavior with Move X + Move Y (tile collision off for missiles/ghosts). True angular homing: the actor flies at constant speed and can only turn so many degrees per update, giving smooth curved pursuit. Speed/turn rate/duration accept variables.",
    type: "actor",
    defaultValue: "$self$",
  },
  {
    key: "targetActorId",
    label: "Home at",
    description: "Actor to home in on (usually the player)",
    type: "actor",
    defaultValue: "player",
  },
  {
    key: "speed",
    label: "Speed",
    description:
      "Flight velocity in subpixels per frame (32 = 1 pixel/frame, max 127)",
    type: "value",
    min: 1,
    max: 127,
    defaultValue: {
      type: "number",
      value: 32,
    },
  },
  {
    key: "turnRate",
    label: "Turn rate",
    description:
      "Max heading change per update, in 256ths of a full turn (4 ≈ 5.6 degrees). Lower = wider curves, easier to dodge.",
    type: "value",
    min: 1,
    max: 128,
    defaultValue: {
      type: "number",
      value: 4,
    },
  },
  {
    key: "duration",
    label: "Duration (frames, 0 = forever)",
    description: "How long to keep homing before continuing",
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
    description: "Frames between steering updates",
    type: "number",
    min: 1,
    max: 16,
    defaultValue: 2,
  },
  {
    key: "aimAtStart",
    label: "Aim at target on start",
    description:
      "Start already pointing at the target. Untick to launch in a fixed direction and curve toward the target from there.",
    type: "checkbox",
    defaultValue: true,
  },
  {
    key: "initialAngle",
    label: "Launch angle (degrees)",
    description: "Initial heading: 0 = up, 90 = right, 180 = down, 270 = left",
    type: "number",
    min: 0,
    max: 359,
    defaultValue: 0,
    conditions: [
      {
        key: "aimAtStart",
        eq: false,
      },
    ],
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
    _ifConst,
    _rpn,
    _actorGetPosition,
    _localRef,
    _performFetchOperations,
    _performValueRPN,
    appendRaw,
    getNextLabel,
  } = helpers;

  const interval = Math.max(
    1,
    Math.min(16, Math.round(Number(input.updateInterval)) || 2),
  );
  let initialAngle = Math.round(Number(input.initialAngle)) || 0;
  initialAngle = Math.max(0, Math.min(359, initialAngle));
  const initialAngleUnits = Math.round((initialAngle * 256) / 360) & 255;

  const actorRef = _declareLocal("tmp0", 1, true);
  const selfPos = _declareLocal("hom_pos_a", 4, true);
  const targetPos = _declareLocal("hom_pos_b", 4, true);
  const dxRef = _declareLocal("hom_dx", 1, true);
  const dyRef = _declareLocal("hom_dy", 1, true);
  const magRef = _declareLocal("hom_mag", 1, true);
  const desiredRef = _declareLocal("hom_desired", 1, true);
  const headingRef = _declareLocal("hom_heading", 1, true);
  const speedRef = _declareLocal("hom_speed", 1, true);
  const turnRef = _declareLocal("hom_turn", 1, true);
  const velXRef = _declareLocal("hom_velx", 1, true);
  const velYRef = _declareLocal("hom_vely", 1, true);
  const durRef = _declareLocal("hom_dur", 1, true);
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

  // desired = atan2 of the (normalized) vector from self to target.
  // The engine's atan2 table only accepts small inputs, so the pixel deltas
  // are scaled down to +-16 preserving their ratio.
  const computeDesired = () => {
    _actorGetPosition(selfPos);
    _actorGetPosition(targetPos);
    _rpn()
      .ref(_localRef(targetPos, 1))
      .ref(_localRef(selfPos, 1))
      .operator(".SUB")
      .int16(32)
      .operator(".DIV")
      .refSet(dxRef)
      .stop();
    _rpn()
      .ref(_localRef(targetPos, 2))
      .ref(_localRef(selfPos, 2))
      .operator(".SUB")
      .int16(32)
      .operator(".DIV")
      .refSet(dyRef)
      .stop();
    _rpn()
      .ref(dxRef)
      .operator(".ABS")
      .ref(dyRef)
      .operator(".ABS")
      .operator(".MAX")
      .int16(1)
      .operator(".MAX")
      .refSet(magRef)
      .stop();
    _rpn()
      .ref(dyRef)
      .int16(16)
      .operator(".MUL")
      .ref(magRef)
      .operator(".DIV")
      .ref(dxRef)
      .int16(16)
      .operator(".MUL")
      .ref(magRef)
      .operator(".DIV")
      .operator(".ATAN2")
      .refSet(desiredRef)
      .stop();
  };

  setActorId(actorRef, input.actorId);
  setActorId(selfPos, input.actorId);
  setActorId(
    targetPos,
    input.targetActorId != null ? input.targetActorId : "player",
  );

  _addComment("Actor Motion: Home At Target");

  evalToLocal(input.speed, speedRef, 32);
  evalToLocal(input.turnRate, turnRef, 4);
  evalToLocal(input.duration, durRef, 0);
  _rpn()
    .ref(speedRef)
    .int16(0)
    .operator(".MAX")
    .int16(127)
    .operator(".MIN")
    .refSet(speedRef)
    .stop();
  _rpn()
    .ref(turnRef)
    .int16(1)
    .operator(".MAX")
    .int16(128)
    .operator(".MIN")
    .refSet(turnRef)
    .stop();

  if (input.aimAtStart !== false) {
    computeDesired();
    _rpn().ref(desiredRef).refSet(headingRef).stop();
  } else {
    _setConst(headingRef, initialAngleUnits);
  }

  const topLabel = getNextLabel();
  _label(topLabel);
  computeDesired();
  // Steer: shortest signed angle difference, clamped to the turn rate
  _rpn()
    .ref(desiredRef)
    .ref(headingRef)
    .operator(".SUB")
    .int16(128)
    .operator(".ADD")
    .int16(255)
    .operator(".B_AND")
    .int16(128)
    .operator(".SUB")
    .ref(turnRef)
    .operator(".MIN")
    .ref(turnRef)
    .operator(".NEG")
    .operator(".MAX")
    .ref(headingRef)
    .operator(".ADD")
    .int16(255)
    .operator(".B_AND")
    .refSet(headingRef)
    .stop();
  // vel = (speed*sin(heading), -speed*cos(heading))
  _rpn().ref(speedRef).refSet(velXRef).stop();
  appendRaw(`VM_SIN_SCALE ${velXRef}, ${headingRef}, 7`);
  _rpn().ref(speedRef).refSet(velYRef).stop();
  appendRaw(`VM_COS_SCALE ${velYRef}, ${headingRef}, 7`);
  _rpn().ref(velYRef).operator(".NEG").refSet(velYRef).stop();
  _stackPush(velYRef);
  _stackPush(velXRef);
  _stackPush(actorRef);
  _callNative("vm_set_actor_velocity");
  _stackPop(3);
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
