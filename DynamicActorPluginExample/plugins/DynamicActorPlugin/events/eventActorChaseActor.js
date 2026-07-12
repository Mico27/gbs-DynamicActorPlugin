const l10n = require("../helpers/l10n").default;

export const id = "EVENT_ACTOR_CHASE_ACTOR";
export const name = "Actor Chase Actor";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = (fetchArg, input) => {
  const verb = input.mode === "flee" ? "flee from" : "chase";
  return `Actor ${fetchArg("actorId")} ${verb} ${fetchArg("targetActorId")}`;
};

export const fields = [
  {
    key: "actorId",
    label: l10n("ACTOR"),
    description: "Actor to steer",
    type: "actor",
    defaultValue: "$self$",
  },
  {
    key: "targetActorId",
    label: "Target actor",
    description: "Actor to chase or flee from",
    type: "actor",
    defaultValue: "player",
  },
  {
    key: "mode",
    label: "Mode",
    description: "Steer toward the target (chase) or away from it (flee)",
    type: "select",
    options: [
      ["chase", "Chase"],
      ["flee", "Flee"],
    ],
    defaultValue: "chase",
  },
  {
    key: "speed",
    label: "Speed",
    description:
      "Movement speed in subpixels per frame (32 = 1 pixel/frame). Also used as the arrival dead zone so the chaser doesn't oscillate on the target.",
    type: "value",
    min: 0,
    max: 127,
    defaultValue: {
      type: "number",
      value: 8,
    },
  },
  {
    key: "stopRange",
    label: "Stop range",
    description:
      "Chase: stops once within this range of the target on every steered axis. Flee: stops once beyond this range on any steered axis. 0 = never stop (chase/flee forever - place in a looping thread or an actor's update script).",
    type: "value",
    min: 0,
    max: 32767,
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
];

export const compile = (input, helpers) => {
  const { _invoke, _stackPush, _stackPushScriptValue, _addComment, _declareLocal, setActorId } = helpers;

  const actorRef = _declareLocal("actorRef", 1, true);
  const targetRef = _declareLocal("targetRef", 1, true);
  setActorId(actorRef, input.actorId);
  setActorId(targetRef, input.targetActorId);

  _addComment("Actor Chase Actor");

  _stackPush(actorRef);
  _stackPush(targetRef);
  _stackPushScriptValue(input.speed || { type: "number", value: 8 });
  _stackPushScriptValue(input.mode === "flee" ? { type: "number", value: 1 } : { type: "number", value: 0 });
  _stackPushScriptValue(input.stopRange || { type: "number", value: 0 });
  
  _invoke("vm_actor_chase_actor", 5, ".ARG4");
};
