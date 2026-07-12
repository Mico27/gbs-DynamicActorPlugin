const l10n = require("../helpers/l10n").default;

export const id = "EVENT_SET_ACTOR_LINKED_ACTOR";
export const name = "Set Actor Parent Actor";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = (fetchArg) => {
  return `Set Actor Parent Actor`;
};

export const fields = [
  {
    key: "actorId",
    label: l10n("ACTOR"),
    description: "Actor that will be given a parent",
    type: "actor",
    defaultValue: "$self$",
  },
  {
    key: "linkedActorId",
    label: "Parent actor",
    description:
      "Actor to follow. From now on, this actor inherits the parent's per-frame movement (tile-collision checked) every frame, in addition to its own behavior physics if it has one - until its parent is changed or cleared.",
    type: "actor",
    defaultValue: "$self$",
  },
];

export const compile = (input, helpers) => {
  const {
    _callNative,
    _stackPush,
    _stackPop,
    _addComment,
    _declareLocal,
    setActorId,
  } = helpers;

  const tmp0 = _declareLocal("tmp0", 1, true);
  const tmp1 = _declareLocal("tmp1", 1, true);

  setActorId(tmp0, input.actorId);
  setActorId(tmp1, input.linkedActorId);

  _addComment("Set Actor Parent Actor");

  _stackPush(tmp1);
  _stackPush(tmp0);

  _callNative("vm_set_actor_parent");
  _stackPop(2);
};


