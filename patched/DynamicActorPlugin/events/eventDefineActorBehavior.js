export const id = "EVENT_DEFINE_ACTOR_BEHAVIOR";
export const name = "Define Actor Behavior";
export const groups = ["EVENT_GROUP_ACTOR"];

// Physics component flags (must match dynamic_actor.h)
const BHV_GRAVITY = 0x01;
const BHV_MOVE_X = 0x02;
const BHV_MOVE_Y = 0x04;
const BHV_LEDGE_STOP = 0x08;
const BHV_REFLECT_X = 0x10;
const BHV_REFLECT_Y = 0x20;
const BHV_LINKED = 0x40;

// Animation component flags
const BHV2_ANIM_FACE = 0x01;
const BHV2_ANIM_IDLE = 0x02;
const BHV2_ANIM_JUMP = 0x04;

const PRESETS = {
  walker: {
    flags: BHV_GRAVITY | BHV_MOVE_X | BHV_MOVE_Y | BHV_REFLECT_X,
    flags2: BHV2_ANIM_FACE | BHV2_ANIM_IDLE | BHV2_ANIM_JUMP,
  },
  walker_ledge: {
    flags: BHV_GRAVITY | BHV_MOVE_X | BHV_MOVE_Y | BHV_REFLECT_X | BHV_LEDGE_STOP,
    flags2: BHV2_ANIM_FACE | BHV2_ANIM_IDLE | BHV2_ANIM_JUMP,
  },
  bouncing_ball: {
    flags: BHV_GRAVITY | BHV_MOVE_X | BHV_MOVE_Y | BHV_REFLECT_X | BHV_REFLECT_Y,
    flags2: BHV2_ANIM_FACE,
  },
  faller: {
    flags: BHV_GRAVITY | BHV_MOVE_Y,
    flags2: BHV2_ANIM_FACE | BHV2_ANIM_IDLE | BHV2_ANIM_JUMP,
  },
  slider: {
    flags: BHV_MOVE_X | BHV_MOVE_Y | BHV_REFLECT_X,
    flags2: BHV2_ANIM_FACE | BHV2_ANIM_IDLE,
  },
  reflector: {
    flags: BHV_MOVE_X | BHV_MOVE_Y | BHV_REFLECT_X | BHV_REFLECT_Y,
    flags2: BHV2_ANIM_FACE,
  },
  attached: {
    flags: BHV_LINKED,
    flags2: 0,
  },
};

export const autoLabel = (fetchArg, input) => {
  const presetLabels = {
    walker: "Walker",
    walker_ledge: "Walker (avoid ledges)",
    bouncing_ball: "Bouncing ball",
    faller: "Falling object",
    slider: "Slider (no gravity)",
    reflector: "Reflector (no gravity)",
    attached: "Attached to actor",
    custom: "Custom",
  };
  return `Define Behavior : ${presetLabels[input.preset] || "Custom"}`;
};

export const fields = [
  {
    key: "behaviorId",
    label: "Behavior slot",
    description:
      "Slot number to store this behavior in (1 to Max behavior slots). Assign it to actors with 'Set Actor Behavior'.",
    type: "value",
    min: 1,
    max: 32,
    defaultValue: {
      type: "number",
      value: 1,
    },
  },
  {
    key: "preset",
    label: "Preset",
    description: "Start from a ready-made behavior or build a custom one from components",
    type: "select",
    options: [
      ["walker", "Walker (gravity, turns at walls)"],
      ["walker_ledge", "Walker (gravity, turns at walls and ledges)"],
      ["bouncing_ball", "Bouncing ball (gravity, bounces off everything)"],
      ["faller", "Falling object (gravity, vertical only)"],
      ["slider", "Slider (no gravity, turns at walls)"],
      ["reflector", "Reflector (no gravity, bounces off everything)"],
      ["attached", "Attached to linked actor"],
      ["custom", "Custom (choose components)"],
    ],
    defaultValue: "walker",
  },
  {
    key: "compGravity",
    label: "Gravity",
    description: "Apply gravity to the actor's vertical velocity each frame",
    type: "checkbox",
    defaultValue: true,
    conditions: [{ key: "preset", eq: "custom" }],
  },
  {
    key: "compMoveX",
    label: "Move horizontally",
    description: "Apply horizontal velocity with wall collision",
    type: "checkbox",
    defaultValue: true,
    conditions: [{ key: "preset", eq: "custom" }],
  },
  {
    key: "compMoveY",
    label: "Move vertically",
    description: "Apply vertical velocity with floor/ceiling collision",
    type: "checkbox",
    defaultValue: true,
    conditions: [{ key: "preset", eq: "custom" }],
  },
  {
    key: "compLedgeStop",
    label: "Turn at ledges",
    description: "While grounded, treat ledges/pits like walls (smart ledge detection)",
    type: "checkbox",
    defaultValue: false,
    conditions: [{ key: "preset", eq: "custom" }],
  },
  {
    key: "compReflectX",
    label: "Turn at walls",
    description: "Reverse horizontal velocity when hitting a wall (otherwise stop)",
    type: "checkbox",
    defaultValue: true,
    conditions: [{ key: "preset", eq: "custom" }],
  },
  {
    key: "compReflectY",
    label: "Bounce on floor/ceiling",
    description: "Bounce vertical velocity when hitting floor or ceiling (otherwise stop)",
    type: "checkbox",
    defaultValue: false,
    conditions: [{ key: "preset", eq: "custom" }],
  },
  {
    key: "compLinked",
    label: "Attach to linked actor",
    description:
      "Follow the linked actor at a fixed offset (set with 'Set Actor Linked Actor'). Overrides all other components.",
    type: "checkbox",
    defaultValue: false,
    conditions: [{ key: "preset", eq: "custom" }],
  },
  {
    key: "animFace",
    label: "Face move direction",
    description: "Face left/right based on horizontal velocity",
    type: "checkbox",
    defaultValue: true,
    conditions: [{ key: "preset", eq: "custom" }],
  },
  {
    key: "animIdle",
    label: "Idle when stopped",
    description: "Play idle animation when horizontal velocity is zero",
    type: "checkbox",
    defaultValue: true,
    conditions: [{ key: "preset", eq: "custom" }],
  },
  {
    key: "animJump",
    label: "Jump animation in air",
    description: "Play jump animation while airborne",
    type: "checkbox",
    defaultValue: true,
    conditions: [{ key: "preset", eq: "custom" }],
  },
  {
    key: "gravity",
    label: "Gravity",
    description: "Acceleration in subpixels per frame (position units: 16 subpixels = 1 pixel)",
    type: "value",
    min: 0,
    max: 255,
    defaultValue: {
      type: "number",
      value: 8,
    },
  },
  {
    key: "maxFallVelocity",
    label: "Max fall speed",
    description: "Maximum downward velocity in subpixels per frame (16 = 1 pixel/frame)",
    type: "value",
    min: 0,
    max: 255,
    defaultValue: {
      type: "number",
      value: 64,
    },
  },
  {
    key: "bounce",
    label: "Bounciness",
    description:
      "Energy kept on each bounce, 0-255 (128 = half, 255 = perfect bounce). Only used with 'Bounce on floor/ceiling'.",
    type: "value",
    min: 0,
    max: 255,
    defaultValue: {
      type: "number",
      value: 128,
    },
  },
];

export const compile = (input, helpers) => {
  const {
    _callNative,
    _stackPush,
    _stackPushConst,
    _stackPop,
    _addComment,
    _declareLocal,
    variableSetToScriptValue,
  } = helpers;

  let flags = 0;
  let flags2 = 0;

  if (input.preset && input.preset !== "custom" && PRESETS[input.preset]) {
    flags = PRESETS[input.preset].flags;
    flags2 = PRESETS[input.preset].flags2;
  } else {
    if (input.compGravity) flags |= BHV_GRAVITY;
    if (input.compMoveX) flags |= BHV_MOVE_X;
    if (input.compMoveY) flags |= BHV_MOVE_Y;
    if (input.compLedgeStop) flags |= BHV_LEDGE_STOP;
    if (input.compReflectX) flags |= BHV_REFLECT_X;
    if (input.compReflectY) flags |= BHV_REFLECT_Y;
    if (input.compLinked) flags |= BHV_LINKED;
    if (input.animFace) flags2 |= BHV2_ANIM_FACE;
    if (input.animIdle) flags2 |= BHV2_ANIM_IDLE;
    if (input.animJump) flags2 |= BHV2_ANIM_JUMP;
  }

  const tmpSlot = _declareLocal("tmp_slot", 1, true);
  const tmpGravity = _declareLocal("tmp_gravity", 1, true);
  const tmpMaxFall = _declareLocal("tmp_max_fall", 1, true);
  const tmpBounce = _declareLocal("tmp_bounce", 1, true);

  variableSetToScriptValue(tmpSlot, input.behaviorId);
  variableSetToScriptValue(tmpGravity, input.gravity || { type: "number", value: 8 });
  variableSetToScriptValue(tmpMaxFall, input.maxFallVelocity || { type: "number", value: 64 });
  variableSetToScriptValue(tmpBounce, input.bounce || { type: "number", value: 128 });

  _addComment(`Define Actor Behavior (flags: ${flags}, anim: ${flags2})`);

  _stackPush(tmpBounce);
  _stackPush(tmpMaxFall);
  _stackPush(tmpGravity);
  _stackPushConst(flags2);
  _stackPushConst(flags);
  _stackPush(tmpSlot);

  _callNative("vm_define_actor_behavior");
  _stackPop(6);
};
