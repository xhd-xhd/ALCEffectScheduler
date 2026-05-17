# ALC Effect Scheduler — Skeleton

This repository contains a lightweight C skeleton for the multi-effect ambient lighting scheduler.

Structure highlights:
- `include/` - shared headers (`types.h`, `led_map.h`, `hal.h`)
- `src/` - modules: CAN parser, events, commands, effects, effect manager, renderer
- `Makefile` - build the example binary `alceffect`

How to build:

```sh
make
./alceffect
```

Built-in effects for demo:
- **ambient** (pri=1) — warm white background on all zones
- **running_light_1** (pri=5) — cyan streaming light on roof front+mid (40+40 px)
- **running_light_2** (pri=6) — orange streaming light on roof mid+rear (40+20 px)
  - rl1 and rl2 overlap on roof-mid: higher-priority rl2 wins the overlap
- **welcome** (pri=7) — blue breathing welcome light on all four doors
- **rear_alert** (pri=10) — red flashing radar alert on rear doors

This is a scaffold. Replace `src/can/parser.c` with your actual CANFD mapping, and extend effects in `src/effects/`.
