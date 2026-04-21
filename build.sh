#!/bin/bash
# Rebuild all srcgame variants into their corresponding Source directories.
# Run from the root of the Playdate_Api_SDL2 repo in an MSYS2/MinGW64 shell.

set -e

MAKE_ARGS="${@}"  # pass any extra make args e.g. DEBUG=1 or -j4

# ============================================================
# Per-game flag lists — add the srcgame dir name to enable
# ============================================================
WALLCLOCK_GAMES=(
    "srcgame_demo-pd-cranktheworld"
    "srcgame_FactoryFarming"
    "srcgame_cascada"
    "srcgame_crank-rhythm"
)

HWRENDER_GAMES=(
    "srcgame_PastaWipeoutPD"

)
# ============================================================

SRCGAME_DIRS=(
    "srcgame_fractal-clock"
    "srcgame_slime_Ascend"
    "srcgame_falling-sand"
    "srcgame_sokoban"
    "srcgame_demo-pd-cranktheworld"
    "srcgame_kuroobi"
    "srcgame_melice-starterkit"    
    "srcgame_cascada"
    "srcgame_FactoryFarming"
    "srcgame_YouCannotGoBack"  
    "srcgame_peanutgb"
    "srcgame_waternet"
    "srcgame_blips"  
    "srcgame_inputtest"
    "srcgame_cranksurvivor"
    "srcgame_anadeken"
    "srcgame_playdators"    
    "srcgame_crank-rhythm"
    "srcgame_lovevania"    
    "srcgame_playdate_pong"    
    "srcgame_bunnymark"
    "srcgame_bunnymark_original"
    "srcgame_PastaWipeoutPD"
    "srcgame_racer"
    "srcgame_tests"
    "srcgame_pdxlander"
    "srcgame_sphere"  
    "srcgame_tetraminosdate"
    "srcgame_deathstarshooter"
    "srcgame_playdoom"
    "srcgame_ddd"
    "srcgame_c_game_engine"
    "srcgame_c_barrel"
    "srcgame_SafetyDiver"
    "srcgame_drifter"
    "srcgame_playdate-slots"
    "srcgame_playdate-poc"
    "srcgame_voxel-terrain-pd"
    "srcgame_mandelbrot-playdate"
    "srcgame_starfield-playdate-c"
    "srcgame_playdate-kaleidoscope"
    "srcgame_scuba-sam"
    "srcgame_3D.playdate"
    "srcgame_PlayInvaders"  
    "srcgame_PlaydateSurvivors"
    "srcgame_Roundering"
    "srcgame_playdate-chip8"
    "srcgame_chip8-Playdate"
    "srcgame-rolling-shooter"
    "srcgame_2048"
    "srcgame_arkanoid"
    "srcgame_blockdude"
    "srcgame_breakout"
    "srcgame_bullethell"
    "srcgame_bumpcars"
    "srcgame_checkers"
    "srcgame_chess"
    "srcgame_collisions"
    "srcgame_dungeoncrawler"
    "srcgame_dynamate"
    "srcgame_fish"
    "srcgame_formula1"
    "srcgame_invaders"
    "srcgame_jap_puzzle"
    "srcgame_kingdom"
    "srcgame_mazethingie"
    "srcgame_pacdate"
    "srcgame_pdpong"
    "srcgame_pebble"
    "srcgame_pinball"
    "srcgame_playingwithblocks"
    "srcgame_playpong"
    "srcgame_pokermagic"
    "srcgame_pong"
    "srcgame_puzzleland"
    "srcgame_puzztrix"
    "srcgame_racer"
    "srcgame_retrotime"
    "srcgame_rubido"
    "srcgame_simplebreakout"
    "srcgame_spaceshipment"
    "srcgame_sprite"
    "srcgame_sprite_collisions"
    "srcgame_squirel"
    "srcgame_stellarator_bifusion"
    "srcgame_witches-playdate"
    "srcgame_tinytrek-playdate"    
)

get_source_dir() {
    local sg="$1"
    if [ "$sg" = "srcgame" ]; then
        echo "Source"
    else
        echo "Source${sg#srcgame}"
    fi
}

in_list() {
    local needle="$1"
    shift
    for item in "$@"; do
        [ "$item" = "$needle" ] && return 0
    done
    return 1
}

find_dirs_with() {
    local root="$1"
    shift
    local patterns=("$@")
    local args=()
    for i in "${!patterns[@]}"; do
        if [ $i -gt 0 ]; then
            args+=(-o)
        fi
        args+=(-name "${patterns[$i]}")
    done
    find "$root" -type f \( "${args[@]}" \) -printf '%h\n' 2>/dev/null | sort -u | tr '\n' ' '
}

STUB_ROOT="src/srcstub"

PASS=0
FAIL=0
SKIP=0
FAILED_GAMES=()

for SRCGAME in "${SRCGAME_DIRS[@]}"; do
    SOURCEDIR="$(get_source_dir "$SRCGAME")"
    GAME_ROOT="src/${SRCGAME}"

    if [ ! -d "${GAME_ROOT}" ]; then
        echo "------------------------------------------------------------"
        echo "SKIP: ${GAME_ROOT} does not exist"
        echo "------------------------------------------------------------"
        SKIP=$((SKIP + 1))
        continue
    fi

    echo "============================================================"
    echo "Building: ${GAME_ROOT} -> ${SOURCEDIR}"
    echo "============================================================"

    EXTRA_CFLAGS="-w -fpermissive"
    EXTRA_MAKE=""

    if in_list "$SRCGAME" "${WALLCLOCK_GAMES[@]}"; then
        EXTRA_CFLAGS="${EXTRA_CFLAGS} -DFILEPLAYER_WALLCLOCK_TRACKING=1"
        echo "  [flag] wallclock tracking enabled"
    fi

    if in_list "$SRCGAME" "${HWRENDER_GAMES[@]}"; then
        EXTRA_MAKE="FORCE_ACCELERATED_RENDER=1"
        echo "  [flag] hardware accelerated render enabled"
    fi

    ALL_C_DIRS="$(find_dirs_with "${GAME_ROOT}" "*.c" "*.h") $(find_dirs_with "${STUB_ROOT}" "*.c" "*.h")"
    ALL_CPP_DIRS="$(find_dirs_with "${GAME_ROOT}" "*.cpp" "*.hpp") $(find_dirs_with "${STUB_ROOT}" "*.cpp" "*.hpp")"
    ALL_C_DIRS="$(echo "$ALL_C_DIRS" | tr ' ' '\n' | sort -u | tr '\n' ' ')"
    ALL_CPP_DIRS="$(echo "$ALL_CPP_DIRS" | tr ' ' '\n' | sort -u | tr '\n' ' ')"

    echo "  SRC_C_DIR  : ${ALL_C_DIRS}"
    echo "  SRC_CPP_DIR: ${ALL_CPP_DIRS}"

    rm -rf ./obj

    if make -s -j16 \
        CFLAGS_EXTRA="${EXTRA_CFLAGS}" \
        LDUSEX11=0 \
        SRC_C_DIR="${ALL_C_DIRS}" \
        SRC_CPP_DIR="${ALL_CPP_DIRS}" \
        OUT_DIR="./${SOURCEDIR}" \
        SOURCE_DIR="${SOURCEDIR}" \
        ${EXTRA_MAKE} \
        ${MAKE_ARGS}; then
        echo "OK: ${SOURCEDIR}"
        PASS=$((PASS + 1))
    else
        echo "FAILED: ${SOURCEDIR}"
        FAIL=$((FAIL + 1))
        FAILED_GAMES+=("${SOURCEDIR}")
    fi
done

echo ""
echo "============================================================"
echo "Build summary: ${PASS} ok, ${FAIL} failed, ${SKIP} skipped"
if [ ${FAIL} -gt 0 ]; then
    echo "Failed games:"
    for g in "${FAILED_GAMES[@]}"; do
        echo "  - ${g}"
    done
fi
echo "============================================================"

[ ${FAIL} -eq 0 ]