const l10n = require("../helpers/l10n").default;

export const id = "EVENT_ACTOR_MOTION_WALL_CRAWL";
export const name = "Actor Motion: Wall Crawl";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = (fetchArg, input) => {
  return `Wall Crawl : ${fetchArg("actorId")}`;
};

export const fields = [
  {
    key: "actorId",
    label: l10n("ACTOR"),
    description:
      "Actor that crawls along walls/ceilings/floors, wrapping around corners (Zelda Spark style). Needs a behavior with Move X + Move Y and tile collision OFF — the crawl logic is what follows the walls. Runs forever; a wall is a fully solid tile (map borders count as wall).",
    type: "actor",
    defaultValue: "$self$",
  },
  {
    key: "wallSide",
    label: "Wall side",
    description:
      "Which hand stays on the wall. Right hand = travels clockwise around solid blocks; left hand = counter-clockwise.",
    type: "select",
    options: [
      ["right", "Right hand (clockwise)"],
      ["left", "Left hand (counter-clockwise)"],
    ],
    defaultValue: "right",
  },
  {
    key: "speed",
    label: "Speed",
    description:
      "Crawl velocity in subpixels per frame (32 = 1 pixel/frame). Snapped to 1/2/4/8/16/32/64 so the actor lands exactly on the 8px cells where turns happen.",
    type: "number",
    min: 1,
    max: 64,
    defaultValue: 16,
  },
  {
    key: "startDirection",
    label: "Start direction",
    description:
      "Initial crawl direction. If it's blocked or has no wall beside it, the crawler turns on the first step automatically.",
    type: "select",
    options: [
      ["right", "Right"],
      ["left", "Left"],
      ["up", "Up"],
      ["down", "Down"],
    ],
    defaultValue: "right",
  },
];

export const compile = (input, helpers) => {
  const {
    _invoke,
    _stackPush,
    _stackPushConst,
    _addComment,
    _declareLocal,
    setActorId,
  } = helpers;

  // Snap speed down to a power of two so it divides 256 (one 8px cell)
  let speed = Math.round(Number(input.speed));
  if (!isFinite(speed)) speed = 16;
  speed = Math.max(1, Math.min(64, speed));
  speed = 1 << Math.floor(Math.log2(speed));

  const side = input.wallSide === "left" ? 1 : 0;
  const dirValues = { up: 0, right: 1, down: 2, left: 3 };
  const startDir =
    dirValues[input.startDirection] !== undefined
      ? dirValues[input.startDirection]
      : 1;

  const actorRef = _declareLocal("tmp0", 1, true);

  setActorId(actorRef, input.actorId);

  _addComment(
    `Actor Motion: Wall Crawl (${side ? "left" : "right"} hand, speed ${speed})`,
  );

  _stackPush(actorRef);
  _stackPushConst(startDir);
  _stackPushConst(side);
  _stackPushConst(speed);
  _invoke("vm_actor_crawl_step", 4, ".ARG3");
};
