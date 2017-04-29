enum boardStates {
  coast,  //coast state
  wait,   //idle state (can't use idle as a word :(
  accel,  //accelerating / driving but no cruise control
  cruise, //cruise control is set
  brake,  //brake state
  boot    //booting up state.
};

static inline char *stringFromEnum(enum boardStates s)
{
    static const char *strings[] = {"idle", "coast", "accel", "cruise", "brake", "boot" /* continue for rest of values */ };

    return strings[s];
}
