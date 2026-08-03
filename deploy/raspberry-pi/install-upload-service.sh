#!/usr/bin/env bash
set -euo pipefail

if [[ ${EUID} -ne 0 ]]; then
    echo "Esegui questo script con sudo." >&2
    exit 1
fi

mela_user=${SUDO_USER:-}
if [[ -z ${mela_user} || ${mela_user} == root ]]; then
    echo "Esegui con: sudo ./deploy/raspberry-pi/install-upload-service.sh" >&2
    exit 1
fi
if [[ ! ${mela_user} =~ ^[a-z_][a-z0-9_-]*$ ]]; then
    echo "Nome utente non valido." >&2
    exit 1
fi

mela_group=$(id -gn "${mela_user}")
mela_home=$(getent passwd "${mela_user}" | cut -d: -f6)
project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)

apt-get update
apt-get install -y python3-venv avahi-daemon

install -d -m 0755 /opt/mela-upload
install -m 0644 "${project_root}/wifi/mela_upload/app.py" /opt/mela-upload/app.py
install -m 0644 "${project_root}/wifi/mela_upload/storage.py" /opt/mela-upload/storage.py
install -m 0644 "${project_root}/wifi/mela_upload/requirements.txt" /opt/mela-upload/requirements.txt
install -d -m 0755 /opt/mela-upload/static
install -m 0644 "${project_root}/wifi/mela_upload/static/index.html" /opt/mela-upload/static/index.html

python3 -m venv /opt/mela-upload/.venv
/opt/mela-upload/.venv/bin/pip install --upgrade pip
/opt/mela-upload/.venv/bin/pip install -r /opt/mela-upload/requirements.txt

install -d -o "${mela_user}" -g "${mela_group}" -m 0755 "${mela_home}/Music/Mela Inbox"
install -d -o "${mela_user}" -g "${mela_group}" -m 0700 "${mela_home}/.config/mela"

unit_file=$(mktemp)
trap 'rm -f "${unit_file}"' EXIT
sed -e "s|@USER@|${mela_user}|g" \
    -e "s|@GROUP@|${mela_group}|g" \
    -e "s|@HOME@|${mela_home}|g" \
    "${project_root}/deploy/raspberry-pi/mela-upload.service.in" \
    > "${unit_file}"
install -m 0644 "${unit_file}" /etc/systemd/system/mela-upload.service
install -m 0644 "${project_root}/deploy/raspberry-pi/mela-upload.avahi.service" \
    /etc/avahi/services/mela-upload.service

systemctl daemon-reload
systemctl enable --now avahi-daemon.service mela-upload.service

echo
echo "Mela Upload installato."
echo "Apri: http://$(hostname).local:8080"
echo "Il PIN sara' mostrato nella pagina WIFI di Mela."
