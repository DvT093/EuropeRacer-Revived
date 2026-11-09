#!/bin/sh
# Bouwt (incrementeel) en start Europe Racer native. Log in run.log.
cd "$(dirname "$0")"
# Speldata (gewijzigde game.ini, saves, crashrapporten) in het project.
export XDG_DATA_HOME="$PWD/userdata"
cmake --build --preset release --target eracer > out_build.log 2>&1 || { grep -m5 -B2 -A6 "error:" out_build.log; exit 1; }
timeout "${ERACER_TIMEOUT:-40}" out/build/release/eracer --game-dir "$PWD/iso/EuropeRacer/Game" "$@" > run.log 2>&1
echo "exit $?" >> run.log
