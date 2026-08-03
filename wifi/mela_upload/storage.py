from __future__ import annotations

import os
import re
import secrets
import unicodedata
from pathlib import Path
from typing import Optional, Set

ALLOWED_EXTENSIONS = {".wav", ".aif", ".aiff", ".flac", ".ogg"}
MAX_UPLOAD_BYTES = 250 * 1024 * 1024


def ensure_private_pin(config_directory: Path) -> str:
    config_directory.mkdir(parents=True, exist_ok=True)
    pin_file = config_directory / "upload-pin.txt"

    if pin_file.is_file():
        pin = pin_file.read_text(encoding="utf-8").strip()
        if re.fullmatch(r"[0-9]{6}", pin):
            return pin

    pin = f"{secrets.randbelow(1_000_000):06d}"
    temporary = config_directory / ".upload-pin.tmp"
    temporary.write_text(pin + "\n", encoding="utf-8")
    os.chmod(temporary, 0o600)
    os.replace(temporary, pin_file)
    return pin


def safe_filename(original_name: str) -> str:
    base_name = Path(original_name).name
    normalised = unicodedata.normalize("NFKD", base_name).encode("ascii", "ignore").decode()
    stem = Path(normalised).stem
    suffix = Path(normalised).suffix.lower()

    if suffix not in ALLOWED_EXTENSIONS:
        raise ValueError("Formato non supportato")

    stem = re.sub(r"[^A-Za-z0-9 _.-]", "_", stem).strip(" ._")
    if not stem:
        stem = "sample"
    return stem[:100] + suffix


def unique_destination(inbox: Path, requested_name: str,
                       reserved_names: Optional[Set[str]] = None) -> Path:
    clean_name = safe_filename(requested_name)
    reserved = reserved_names or set()
    destination = inbox / clean_name
    counter = 2
    while destination.exists() or destination.name in reserved:
        destination = inbox / f"{Path(clean_name).stem}-{counter}{Path(clean_name).suffix}"
        counter += 1
    return destination


def resolve_inbox_file(inbox: Path, file_name: str) -> Path:
    candidate = (inbox / Path(file_name).name).resolve()
    if candidate.parent != inbox.resolve() or candidate.suffix.lower() not in ALLOWED_EXTENSIONS:
        raise ValueError("File non valido")
    return candidate
