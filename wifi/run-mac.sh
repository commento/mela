#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/mela_upload" && pwd)
venv_dir="${script_dir}/.venv"

if [[ ! -x "${venv_dir}/bin/python3" ]]; then
    python3 -m venv "${venv_dir}"
fi

if ! "${venv_dir}/bin/python3" -c 'import fastapi, multipart, uvicorn' >/dev/null 2>&1; then
    "${venv_dir}/bin/pip" install -r "${script_dir}/requirements.txt"
fi

echo "Mela Upload attivo su http://localhost:8080"
echo "Lascia aperta questa finestra durante il test."
exec "${venv_dir}/bin/uvicorn" app:app \
    --app-dir "${script_dir}" \
    --host 0.0.0.0 \
    --port 8080 \
    --workers 1
