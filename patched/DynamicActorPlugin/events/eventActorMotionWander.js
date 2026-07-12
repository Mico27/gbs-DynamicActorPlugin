const l10n = require("../helpers/l10n").default;

export const id = "EVENT_ACTOR_MOTION_WANDER";
export const name = "Actor Motion: Random Wander Step";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = (fetchArg, input) => {
  return `Wander Step : ${fetchArg("actorId")}`;
};

export const fields = [
  {
    key: "actorId",
    label: l10n("ACTOR"),
    description:
      "Actor to wander. Needs a behavior with Move X / Move Y enabled (4-direction facing animation recommended). One event = one step: pick a random direction, walk, pause — place it in a loop.",
    type: "actor",
    defaultValue: "$self$",
  },
  {
    key: "speed",
    label: "Speed",
    description:
      "Walk velocity in subpixels per frame (32 = 1 pixel/frame, max 127)",
    type: "number",
    min: 1,
    max: 127,
    defaultValue: 16,
  },
  {
    key: "moveFrames",
    label: "Walk time (frames)",
    description: "Frames to walk in the chosen direction",
    type: "number",
    min: 1,
    max: 600,
    defaultValue: 32,
  },
  {
    key: "pauseFrames",
    label: "Pause time (frames)",
    description: "Frames to stand still after walking",
    type: "number",
    min: 0,
    max: 600,
    defaultValue: 24,
  },
  {
    key: "includeIdle",
    label: "Sometimes stay put",
    description:
      "Adds 'don't move' as a fifth possible outcome for a more natural wander",
    type: "checkbox",
    defaultValue: false,
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
    _rand,
    _randomize,
    getNextLabel,
  } = helpers;

  const clampInt = (v, min, max, dflt) => {
    const n = Math.round(Number(v));
    if (!isFinite(n)) return dflt;
    return Math.max(min, Math.min(max, n));
  };

  const speed = clampInt(input.speed, 1, 127, 16);
  const moveFrames = clampInt(input.moveFrames, 1, 600, 32);
  const pauseFrames = clampInt(input.pauseFrames, 0, 600, 24);
  const includeIdle = input.includeIdle === true;

  const actorRef = _declareLocal("tmp0", 1, true);
  const dirRef = _declareLocal("wander_dir", 1, true);
  const waitArgsRef = _declareLocal("wait_args", 1, true);

  setActorId(actorRef, input.actorId);

  _addComment("Actor Motion: Random Wander Step");

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

  _randomize();
  _rand(dirRef, 0, includeIdle ? 5 : 4);

  const upLabel = getNextLabel();
  const downLabel = getNextLabel();
  const leftLabel = getNextLabel();
  const joinLabel = getNextLabel();

  _ifConst(".EQ", dirRef, 0, upLabel, 0);
  _ifConst(".EQ", dirRef, 1, downLabel, 0);
  _ifConst(".EQ", dirRef, 2, leftLabel, 0);
  if (includeIdle) {
    const rightLabel = getNextLabel();
    _ifConst(".EQ", dirRef, 3, rightLabel, 0);
    setVelXY(0, 0); // stay put
    _jump(joinLabel);
    _label(rightLabel);
  }
  setVelXY(speed, 0);
  _jump(joinLabel);
  _label(upLabel);
  setVelXY(0, -speed);
  _jump(joinLabel);
  _label(downLabel);
  setVelXY(0, speed);
  _jump(joinLabel);
  _label(leftLabel);
  setVelXY(-speed, 0);
  _label(joinLabel);

  waitN(moveFrames);
  setVelXY(0, 0);
  waitN(pauseFrames);

  _addNL();
};
