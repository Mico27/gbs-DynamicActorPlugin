const WAIT_COL_H = 0x01;
const WAIT_COL_V = 0x02;
const WAIT_COL_PIT = 0x04;
const WAIT_COL_ACTOR = 0x08;

export const id = "EVENT_WAIT_FOR_ACTOR_COLLISION_BY_INDEX";
export const name = "Wait For Actor Collision By Index";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = (fetchArg) => {
  return `Wait For Actor Collision : ${fetchArg("actorIndex")}`;
};

export const fields = [
  {
    key: "actorIndex",
    label: "Actor Index",
    description: "Index of the actor to monitor for collision.",
    type: "value",
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
  {
    key: "waitHorizontal",
    label: "Horizontal tile collision",
    description: "Wait for wall collision from X movement",
    type: "checkbox",
    defaultValue: true,
  },
  {
    key: "waitVertical",
    label: "Vertical tile collision",
    description: "Wait for floor/ceiling collision from Y movement",
    type: "checkbox",
    defaultValue: true,
  },
  {
    key: "waitPit",
    label: "Pit / ledge collision",
    description: "Wait for ledge-stop collision from X movement",
    type: "checkbox",
    defaultValue: false,
  },
  {
    key: "waitActor",
    label: "Actor collision",
    description: "Wait for collision with another collidable actor",
    type: "checkbox",
    defaultValue: false,
  },
];

export const compile = (input, helpers) => {
  const { _stackPushScriptValue, _stackPushConst, _addComment, _invoke } = helpers;

  let flags = 0;
  if (input.waitHorizontal) flags |= WAIT_COL_H;
  if (input.waitVertical) flags |= WAIT_COL_V;
  if (input.waitPit) flags |= WAIT_COL_PIT;
  if (input.waitActor) flags |= WAIT_COL_ACTOR;

  _addComment("Wait For Actor Collision By Index");
  _stackPushScriptValue(input.actorIndex);
  _stackPushConst(flags);  
  _invoke("vm_wait_for_collision", 2, ".ARG1");
};
