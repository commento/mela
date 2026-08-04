#!/usr/bin/env bash
set -euo pipefail

if [[ ${EUID} -ne 0 ]]; then
    echo "Esegui questo script con sudo." >&2
    exit 1
fi

state_directory=/var/lib/mela-kiosk
systemctl disable --now mela-kiosk.service 2>/dev/null || true
systemctl unmask getty@tty1.service

if [[ -f ${state_directory}/previous-target ]]; then
    previous_target=$(tr -d '\n' < "${state_directory}/previous-target")
    if [[ ${previous_target} =~ ^[a-zA-Z0-9_.@-]+\.target$ ]]; then
        systemctl set-default "${previous_target}"
    fi
fi

if [[ -f ${state_directory}/display-manager-was-enabled \
      && -f ${state_directory}/display-manager ]]; then
    display_manager=$(tr -d '\n' < "${state_directory}/display-manager")
    if [[ ${display_manager} =~ ^[a-zA-Z0-9_.@-]+\.service$ ]]; then
        systemctl enable "${display_manager}"
    fi
fi

systemctl daemon-reload
echo "Desktop ripristinato per il prossimo riavvio."
echo "Riavvia con: sudo systemctl reboot"
