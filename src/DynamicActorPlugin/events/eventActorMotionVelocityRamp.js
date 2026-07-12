const l10n = require("../helpers/l10n").default;

export const id = "EVENT_ACTOR_MOTION_VELOCITY_RAMP";
export const name = "Actor Motion: Accelerate / Decelerate";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = (fetchArg, input) => {
  return `Accelerate To : ${fetchArg("targetVelocity")}`;
};

export const fields = [
  {
    key: "actorId",
    label: l10n("ACTOR"),
    description: "Actor to move. Needs a behavior with Move X / Move Y enabled.",
    type: "actor",
    defaultValue: "$self$",
  },
  {
    key: "axis",
    label: "Axis",
    description: "Which velocity to ramp",
    type: "select",
    options: [
      ["x", "Horizontal (X)"],
      ["y", "Vertical (Y)"],
    ],
    defaultValue: "x",
  },
  {
    key: "targetVelocity",
    label: "Target velocity",
    description:
      "Velocity to ramp to, in subpixels per frame (32 = 1 pixel/frame, negative = left/up, max ±127). Use 0 to decelerate to a stop.",
    type: "number",
    min: -127,
    max: 127,
    defaultValue: 32,
  },
  {
    key: "acceleration",
    label: "Acceleration (subpx/frame per step)",
    description:
      "How much the velocity changes per update step. Bigger = reaches the target faster.",
    type: "number",
    min: 1,
    max: 127,
    defaultValue: 2,
  },
  {
    key: "stepFrames",
    label: "Update every (frames)",
    description: "Frames between velocity updates",
    type: "number",
    min: 1,
    max: 32,
    defaultValue: 1,
  },
  {
    key: "waitUntilDone",
    label: "Wait until target reached",
    description:
      "Keep ramping until the velocity equals the target. The ramp re-reads the actor's velocity each step, so wall hits that zero the velocity are ramped back up. Don't combine with a chase/flee behavior on the same axis.",
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
    getNextLabel,
  } = helpers;

  const clampInt = (v, min, max, dflt) => {
    const n = Math.round(Number(v));
    if (!isFinite(n)) return dflt;
    return Math.max(min, Math.min(max, n));
  };

  const target = clampInt(input.targetVelocity, -127, 127, 32);
  const accel = clampInt(input.acceleration, 1, 127, 2);
  const stepFrames = clampInt(input.stepFrames, 1, 32, 1);
  const axis = input.axis === "y" ? "y" : "x";
  const getNative =
    axis === "x" ? "vm_get_actor_velocity_x" : "vm_get_actor_velocity_y";
  const setNative =
    axis === "x" ? "vm_set_actor_velocity_x" : "vm_set_actor_velocity_y";

  const actorRef = _declareLocal("tmp0", 1, true);
  const velRef = _declareLocal("ramp_vel", 1, true);
  const waitArgsRef = _declareLocal("wait_args", 1, true);

  setActorId(actorRef, input.actorId);

  _addComment(`Actor Motion: Accelerate ${axis} to ${target}`);

  const waitN = (frames) => {
    if (frames <= 0) return;
    if (frames < 5) {
      for (let i = 0; i < frames; i++) _idle();
    } else {
      _setConst(waitArgsRef, frames);
      _invoke("wait_frames", 0, waitArgsRef);
    }
  };

  const readVel = () => {
    _stackPushConst(velRef);
    _stackPush(actorRef);
    _callNative(getNative);
    _stackPop(2);
  };

  const writeVel = () => {
    _stackPush(velRef);
    _stackPush(actorRef);
    _callNative(setNative);
    _stackPop(2);
  };

  const stepVel = () => {
    // vel += clamp(target - vel, -accel, +accel) — lands exactly on target
    _rpn()
      .ref(velRef)
      .int16(target)
      .ref(velRef)
      .operator(".SUB")
      .int16(accel)
      .operator(".MIN")
      .int16(-accel)
      .operator(".MAX")
      .operator(".ADD")
      .refSet(velRef)
      .stop();
  };

  if (input.waitUntilDone !== false) {
    const topLabel = getNextLabel();
    const doneLabel = getNextLabel();
    _label(topLabel);
    readVel();
    _ifConst(".EQ", velRef, target, doneLabel, 0);
    stepVel();
    writeVel();
    waitN(stepFrames);
    _jump(topLabel);
    _label(doneLabel);
  } else {
    // Single step (place inside your own loop)
    readVel();
    stepVel();
    writeVel();
    waitN(stepFrames);
  }

  _addNL();
};
