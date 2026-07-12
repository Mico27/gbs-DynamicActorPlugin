const l10n = require("../helpers/l10n").default;

export const id = "EVENT_ACTOR_MOTION_HOP";
export const name = "Actor Motion: Hop";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = (fetchArg, input) => {
  return `Hop Motion : ${fetchArg("actorId")}`;
};

export const fields = [
  {
    key: "actorId",
    label: l10n("ACTOR"),
    description:
      "Actor to hop. Needs a behavior with Gravity, Move X and Move Y enabled (e.g. the walker preset) — landing is detected from the behavior's grounded state.",
    type: "actor",
    defaultValue: "$self$",
  },
  {
    key: "jumpVelocity",
    label: "Jump strength",
    description:
      "Upward launch velocity in subpixels per frame (32 = 1 pixel/frame, max 127)",
    type: "number",
    min: 1,
    max: 127,
    defaultValue: 96,
  },
  {
    key: "xVelocity",
    label: "X velocity",
    description:
      "Horizontal velocity during the hop, in subpixels per frame (negative = left, 0 = hop in place, max ±127). With direction 'Toward/Away from actor' only its size is used.",
    type: "number",
    min: -127,
    max: 127,
    defaultValue: 16,
  },
  {
    key: "direction",
    label: "Direction",
    description: "How the horizontal hop direction is chosen",
    type: "select",
    options: [
      ["fixed", "Fixed (use X velocity sign)"],
      ["toward", "Toward actor"],
      ["away", "Away from actor"],
    ],
    defaultValue: "fixed",
  },
  {
    key: "targetActorId",
    label: "Target actor",
    description: "Actor to hop toward / away from",
    type: "actor",
    defaultValue: "player",
    conditions: [
      {
        key: "direction",
        ne: "fixed",
      },
    ],
  },
  {
    key: "landPause",
    label: "Pause on landing (frames)",
    description: "Frames to rest after landing before the event finishes",
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
    _if,
    _ifConst,
    _actorGetPosition,
    _localRef,
    getNextLabel,
  } = helpers;

  const clampInt = (v, min, max, dflt) => {
    const n = Math.round(Number(v));
    if (!isFinite(n)) return dflt;
    return Math.max(min, Math.min(max, n));
  };

  const jumpVel = clampInt(input.jumpVelocity, 1, 127, 96);
  const xVel = clampInt(input.xVelocity, -127, 127, 16);
  const landPause = clampInt(input.landPause, 0, 600, 30);
  const direction =
    input.direction === "toward" || input.direction === "away"
      ? input.direction
      : "fixed";

  const actorRef = _declareLocal("tmp0", 1, true);
  const stateRef = _declareLocal("bhv_state", 1, true);
  const waitArgsRef = _declareLocal("wait_args", 1, true);

  setActorId(actorRef, input.actorId);

  _addComment(`Actor Motion: Hop (${direction})`);

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

  // Launch
  if (direction === "fixed" || xVel === 0) {
    setVelXY(xVel, -jumpVel);
  } else {
    const selfPos = _declareLocal("hop_pos_a", 4, true);
    const targetPos = _declareLocal("hop_pos_b", 4, true);
    setActorId(selfPos, input.actorId);
    setActorId(targetPos, input.targetActorId != null ? input.targetActorId : "player");
    _actorGetPosition(selfPos);
    _actorGetPosition(targetPos);

    const speed = Math.abs(xVel);
    // Target left of self -> toward = negative x
    const leftVel = direction === "toward" ? -speed : speed;
    const negLabel = getNextLabel();
    const joinLabel = getNextLabel();
    _if(".LT", _localRef(targetPos, 1), _localRef(selfPos, 1), negLabel, 0);
    setVelXY(-leftVel, -jumpVel);
    _jump(joinLabel);
    _label(negLabel);
    setVelXY(leftVel, -jumpVel);
    _label(joinLabel);
  }

  // Let the behavior update flip the state to airborne before checking landing
  _idle();
  _idle();

  // Wait until grounded again (behavior state 1)
  const waitLabel = getNextLabel();
  const landedLabel = getNextLabel();
  _label(waitLabel);
  _stackPushConst(stateRef);
  _stackPush(actorRef);
  _callNative("vm_get_actor_state");
  _stackPop(2);
  _ifConst(".EQ", stateRef, 1, landedLabel, 0);
  _idle();
  _jump(waitLabel);
  _label(landedLabel);

  // Stop horizontal movement and rest
  _stackPushConst(0);
  _stackPush(actorRef);
  _callNative("vm_set_actor_velocity_x");
  _stackPop(2);
  waitN(landPause);

  _addNL();
};
