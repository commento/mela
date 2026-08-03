from __future__ import annotations

import asyncio
import os
import secrets
from pathlib import Path
from typing import Optional

from fastapi import FastAPI, Header, HTTPException, UploadFile
from fastapi.responses import FileResponse

from storage import (
    ALLOWED_EXTENSIONS,
    MAX_UPLOAD_BYTES,
    ensure_private_pin,
    resolve_inbox_file,
    unique_destination,
)

ROOT = Path(__file__).resolve().parent
INBOX = Path(os.environ.get("MELA_INBOX", Path.home() / "Music" / "Mela Inbox"))
CONFIG = Path(os.environ.get("MELA_CONFIG", Path.home() / ".config" / "mela"))
INBOX.mkdir(parents=True, exist_ok=True)
PIN = ensure_private_pin(CONFIG)
UPLOAD_NAME_LOCK = asyncio.Lock()
RESERVED_NAMES: set[str] = set()

app = FastAPI(title="Mela Wi-Fi Upload", docs_url=None, redoc_url=None)


def require_pin(x_mela_pin: Optional[str]) -> None:
    if x_mela_pin is None or not secrets.compare_digest(x_mela_pin, PIN):
        raise HTTPException(status_code=401, detail="PIN non valido")


@app.get("/")
async def index() -> FileResponse:
    return FileResponse(ROOT / "static" / "index.html")


@app.get("/api/health")
async def health() -> dict[str, str]:
    return {"status": "ok"}


@app.get("/api/files")
async def list_files(x_mela_pin: Optional[str] = Header(default=None)) -> dict[str, object]:
    require_pin(x_mela_pin)
    files = []
    for path in sorted(INBOX.iterdir(), key=lambda item: item.stat().st_mtime, reverse=True):
        if path.is_file() and not path.is_symlink() and path.suffix.lower() in ALLOWED_EXTENSIONS:
            stat = path.stat()
            files.append({"name": path.name, "size": stat.st_size, "modified": stat.st_mtime})
    return {"files": files, "maxUploadBytes": MAX_UPLOAD_BYTES}


@app.post("/api/upload", status_code=201)
async def upload_sample(file: UploadFile,
                        x_mela_pin: Optional[str] = Header(default=None)) -> dict[str, object]:
    require_pin(x_mela_pin)
    if not file.filename:
        raise HTTPException(status_code=400, detail="Nome file mancante")

    try:
        async with UPLOAD_NAME_LOCK:
            destination = unique_destination(INBOX, file.filename, RESERVED_NAMES)
            RESERVED_NAMES.add(destination.name)
    except ValueError as error:
        raise HTTPException(status_code=415, detail=str(error)) from error

    temporary = INBOX / f".{secrets.token_hex(12)}.part"
    total = 0
    try:
        with temporary.open("xb") as output:
            while chunk := await file.read(1024 * 1024):
                total += len(chunk)
                if total > MAX_UPLOAD_BYTES:
                    raise HTTPException(status_code=413, detail="File oltre il limite di 250 MB")
                output.write(chunk)
            output.flush()
            os.fsync(output.fileno())

        if total == 0:
            raise HTTPException(status_code=400, detail="Il file e' vuoto")
        os.replace(temporary, destination)
    finally:
        await file.close()
        temporary.unlink(missing_ok=True)
        async with UPLOAD_NAME_LOCK:
            RESERVED_NAMES.discard(destination.name)

    return {"name": destination.name, "size": total}


@app.delete("/api/files/{file_name}")
async def delete_file(file_name: str,
                      x_mela_pin: Optional[str] = Header(default=None)) -> dict[str, bool]:
    require_pin(x_mela_pin)
    try:
        target = resolve_inbox_file(INBOX, file_name)
    except ValueError as error:
        raise HTTPException(status_code=400, detail=str(error)) from error
    if not target.is_file() or target.is_symlink():
        raise HTTPException(status_code=404, detail="File non trovato")
    target.unlink()
    return {"deleted": True}
