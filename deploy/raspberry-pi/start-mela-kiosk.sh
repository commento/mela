#!/usr/bin/env bash
set -u

display_mode=${MELA_DISPLAY_MODE:-1920x1200}
display_output=

# Xorg can take a moment to publish its outputs when this script is started by
# xinit. Wait briefly, then select the first connected monitor (the kiosk uses
# a single touch display).
for _ in $(seq 1 20); do
    display_output=$(xrandr --query 2>/dev/null \
        | awk '$2 == "connected" { print $1; exit }')
    if [[ -n ${display_output} ]]; then
        break
    fi
    sleep 0.1
done

if [[ -n ${display_output} ]]; then
    if xrandr --output "${display_output}" --mode "${display_mode}"; then
        echo "Mela: display ${display_output} impostato a ${display_mode}."
    else
        echo "Mela: il display ${display_output} non espone il mode ${display_mode}; uso il mode corrente." >&2
        xrandr --query >&2 || true
    fi
else
    echo "Mela: nessun display X11 rilevato; uso la configurazione corrente." >&2
fi

exec /opt/mela/bin/Mela "$@"
