const l10n = require("../helpers/l10n").default;

export const id = "EVENT_ACTOR_MOTION_ZIGZAG";
export const name = "Actor Motion: Zig Zag";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = (fetchArg, input) => {
  return `Zig Zag Motion : ${fetchArg("actorId")}`;
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
    key: "xVelocity",
    label: "X velocity",
    description:
      "Horizontal velocity in subpixels per frame (32 = 1 pixel/frame, negative = left, max ±127)",
    type: "number",
    min: -127,
    max: 127,
    defaultValue: -16,
  },
  {
    key: "yVelocity",
    label: "Y velocity",
    description:
      "Vertical velocity in subpixels per frame (32 = 1 pixel/frame, negative = up, max ±127). Its sign sets the first leg's direction.",
    type: "number",
    min: -127,
    max: 127,
    defaultValue: 16,
  },
  {
    key: "legFrames",
    label: "Leg length (frames)",
    description: "Frames per zig (and per zag)",
    type: "number",
    min: 1,
    max: 600,
    defaultValue: 30,
  },
  {
    key: "flip",
    label: "Alternate",
    description:
      "Which velocity flips sign between legs. Vertical = classic horizontal zig zag; Horizontal = vertical zig zag; Both = diagonal bounce.",
    type: "select",
    options: [
      ["y", "Vertical (Y) flips"],
      ["x", "Horizontal (X) flips"],
      ["both", "Both flip"],
    ],
    defaultValue: "y",
  },
  {
    key: "cycles",
    label: "Cycles (0 = forever)",
    description: "Number of zig+zag pairs to run before continuing",
    type: "number",
    min: 0,
    max: 255,
    defaultValue: 1,
  },
  {
    key: "stopAtEnd",
    label: "Stop at end",
    description:
      "Zero the velocity when done. Turn off to chain seamlessly into another motion.",
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
    _loop,
    getNextLabel,
  } = helpers;

  const clampInt = (v, min, max, dflt) => {
    const n = Math.round(Number(v));
    if (!isFinite(n)) return dflt;
    return Math.max(min, Math.min(max, n));
  };

  const vx = clampInt(input.xVelocity, -127, 127, -16);
  const vy = clampInt(input.yVelocity, -127, 127, 16);
  const legFrames = clampInt(input.legFrames, 1, 600, 30);
  const cycles = clampInt(input.cycles, 0, 255, 1);
  const flip = input.flip === "x" || input.flip === "both" ? input.flip : "y";

  const vx2 = flip === "x" || flip === "both" ? -vx : vx;
  const vy2 = flip === "y" || flip === "both" ? -vy : vy;

  const actorRef = _declareLocal("tmp0", 1, true);
  const waitArgsRef = _declareLocal("wait_args", 1, true);

  setActorId(actorRef, input.actorId);

  _addComment(`Actor Motion: Zig Zag (${vx},${vy} / ${vx2},${vy2})`);

  const setVelXY = (x, y) => {
    _stackPushConst(y);
    _stackPushConst(x);
    _stackPush(actorRef);
    _callNative("vm_set_actor_velocity");
    _stackPop(3);
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

  const emitCycle = () => {
    setVelXY(vx, vy);
    waitN(legFrames);
    setVelXY(vx2, vy2);
    waitN(legFrames);
  };

  if (cycles === 0) {
    const topLabel = getNextLabel();
    _label(topLabel);
    emitCycle();
    _jump(topLabel);
  } else if (cycles === 1) {
    emitCycle();
  } else {
    const counterRef = _declareLocal("cycle_count", 1, true);
    _setConst(counterRef, cycles - 1);
    const topLabel = getNextLabel();
    _label(topLabel);
    emitCycle();
    _loop(counterRef, topLabel, 0);
  }

  if (cycles !== 0 && input.stopAtEnd !== false) {
    setVelXY(0, 0);
  }

  _addNL();
};
