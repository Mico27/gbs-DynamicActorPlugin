const l10n = require("../helpers/l10n").default;

const WAIT_COL_V = 0x02;

export const id = "EVENT_ACTOR_MOTION_THWOMP";
export const name = "Actor Motion: Thwomp Slam";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = (fetchArg, input) => {
  return `Thwomp Slam : ${fetchArg("actorId")}`;
};

export const fields = [
  {
    key: "actorId",
    label: l10n("ACTOR"),
    description:
      "Actor that slams. Needs a behavior with Move Y enabled and NO gravity (constant slam/rise speed). One event = one full cycle: watch, slam to the floor, pause, rise to the ceiling — place it in a loop.",
    type: "actor",
    defaultValue: "$self$",
  },
  {
    key: "targetActorId",
    label: "Watch for",
    description: "Actor that triggers the slam (usually the player)",
    type: "actor",
    defaultValue: "player",
  },
  {
    key: "detectRange",
    label: "Trigger range (px)",
    description:
      "Horizontal distance at which the slam triggers, in pixels",
    type: "number",
    min: 1,
    max: 160,
    defaultValue: 16,
  },
  {
    key: "requireBelow",
    label: "Only when target is below",
    description: "Only slam when the watched actor is below this actor",
    type: "checkbox",
    defaultValue: true,
  },
  {
    key: "slamVelocity",
    label: "Slam speed",
    description:
      "Downward slam velocity in subpixels per frame (32 = 1 pixel/frame, max 127). For an accelerating slam, precede with Accelerate / Decelerate instead.",
    type: "number",
    min: 1,
    max: 127,
    defaultValue: 120,
  },
  {
    key: "floorPause",
    label: "Pause on floor (frames)",
    description: "Frames to sit on the floor after the slam",
    type: "number",
    min: 0,
    max: 600,
    defaultValue: 40,
  },
  {
    key: "riseVelocity",
    label: "Rise speed",
    description:
      "Upward return velocity in subpixels per frame (rises until it hits the ceiling / its resting tile, max 127)",
    type: "number",
    min: 1,
    max: 127,
    defaultValue: 32,
  },
  {
    key: "topPause",
    label: "Pause at top (frames)",
    description: "Frames to wait after returning to the top",
    type: "number",
    min: 0,
    max: 600,
    defaultValue: 30,
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
    _actorGetPosition,
    _localRef,
    getNextLabel,
  } = helpers;

  const clampInt = (v, min, max, dflt) => {
    const n = Math.round(Number(v));
    if (!isFinite(n)) return dflt;
    return Math.max(min, Math.min(max, n));
  };

  const detectRange = clampInt(input.detectRange, 1, 160, 16);
  const slamVel = clampInt(input.slamVelocity, 1, 127, 120);
  const riseVel = clampInt(input.riseVelocity, 1, 127, 32);
  const floorPause = clampInt(input.floorPause, 0, 600, 40);
  const topPause = clampInt(input.topPause, 0, 600, 30);

  const actorRef = _declareLocal("tmp0", 1, true);
  const selfPos = _declareLocal("thw_pos_a", 4, true);
  const targetPos = _declareLocal("thw_pos_b", 4, true);
  const flagRef = _declareLocal("thw_flag", 1, true);
  const waitArgsRef = _declareLocal("wait_args", 1, true);

  setActorId(actorRef, input.actorId);
  setActorId(selfPos, input.actorId);
  setActorId(
    targetPos,
    input.targetActorId != null ? input.targetActorId : "player",
  );

  _addComment("Actor Motion: Thwomp Slam");

  const setVelY = (v) => {
    _stackPushConst(v);
    _stackPush(actorRef);
    _callNative("vm_set_actor_velocity_y");
    _stackPop(2);
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

  const waitForVerticalCollision = () => {
    _stackPushConst(WAIT_COL_V);
    _stackPush(actorRef);
    _invoke("vm_wait_for_collision", 2, ".ARG1");
  };

  // Watch: idle until the target is horizontally in range (and below, if set)
  const detectLabel = getNextLabel();
  const slamLabel = getNextLabel();
  _label(detectLabel);
  _actorGetPosition(selfPos);
  _actorGetPosition(targetPos);
  const rpn = _rpn()
    .ref(_localRef(selfPos, 1))
    .ref(_localRef(targetPos, 1))
    .operator(".SUB")
    .operator(".ABS")
    .int16(detectRange * 32)
    .operator(".LTE");
  if (input.requireBelow !== false) {
    rpn
      .ref(_localRef(targetPos, 2))
      .ref(_localRef(selfPos, 2))
      .operator(".GT")
      .operator(".AND");
  }
  rpn.refSet(flagRef).stop();
  _ifConst(".EQ", flagRef, 1, slamLabel, 0);
  _idle();
  _jump(detectLabel);

  // Slam down until the floor stops us
  _label(slamLabel);
  setVelY(slamVel);
  waitForVerticalCollision();
  setVelY(0);
  waitN(floorPause);

  // Rise until the ceiling / resting tile stops us
  setVelY(-riseVel);
  waitForVerticalCollision();
  setVelY(0);
  waitN(topPause);

  _addNL();
};
