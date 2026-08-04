#!/usr/bin/env bash
set -euo pipefail

if [[ ${EUID} -ne 0 ]]; then
    echo "Esegui questo script con sudo." >&2
    exit 1
fi

mela_user=${SUDO_USER:-}
if [[ -z ${mela_user} || ${mela_user} == root ]]; then
    echo "Esegui con: sudo ./deploy/raspberry-pi/install-kiosk-service.sh" >&2
    exit 1
fi
if [[ ! ${mela_user} =~ ^[a-z_][a-z0-9_-]*$ ]]; then
    echo "Nome utente non valido." >&2
    exit 1
fi

mela_group=$(id -gn "${mela_user}")
mela_home=$(getent passwd "${mela_user}" | cut -d: -f6)
project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
mela_binary=${MELA_BINARY:-${project_root}/build-pi/Mela_artefacts/Release/Mela}
state_directory=/var/lib/mela-kiosk

if [[ ! -x ${mela_binary} ]]; then
    echo "Binario Release non trovato: ${mela_binary}" >&2
    echo "Compila prima con cmake --build build-pi --parallel 3" >&2
    exit 1
fi

apt-get update
apt-get install -y xserver-xorg-core xserver-xorg-input-libinput xinit x11-xserver-utils

install -d -m 0755 /opt/mela/bin
install -m 0755 "${mela_binary}" /opt/mela/bin/Mela
install -d -m 0700 "${state_directory}"

for hardware_group in audio video input render; do
    if getent group "${hardware_group}" >/dev/null; then
        usermod -aG "${hardware_group}" "${mela_user}"
    fi
done

if [[ ! -f ${state_directory}/previous-target ]]; then
    systemctl get-default > "${state_directory}/previous-target"
fi

display_manager=$(systemctl show display-manager.service --property=Id --value 2>/dev/null || true)
if [[ -n ${display_manager} && ${display_manager} != display-manager.service ]]; then
    printf '%s\n' "${display_manager}" > "${state_directory}/display-manager"
    if systemctl is-enabled "${display_manager}" >/dev/null 2>&1; then
        touch "${state_directory}/display-manager-was-enabled"
        systemctl disable "${display_manager}"
    fi
fi

unit_file=$(mktemp)
trap 'rm -f "${unit_file}"' EXIT
sed -e "s|@USER@|${mela_user}|g" \
    -e "s|@GROUP@|${mela_group}|g" \
    -e "s|@HOME@|${mela_home}|g" \
    "${project_root}/deploy/raspberry-pi/mela-kiosk.service.in" \
    > "${unit_file}"
install -m 0644 "${unit_file}" /etc/systemd/system/mela-kiosk.service

systemctl set-default multi-user.target
systemctl mask getty@tty1.service
systemctl daemon-reload
systemctl enable mela-kiosk.service

echo
echo "Modalita kiosk installata. La sessione attuale non verra' interrotta."
echo "Al prossimo riavvio Mela sara' l'unica app grafica su tty1."
echo "Riavvia con: sudo systemctl reboot"
echo "Ripristino desktop: sudo ./deploy/raspberry-pi/restore-desktop.sh"
