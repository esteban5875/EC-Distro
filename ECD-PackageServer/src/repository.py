import json
import os
import shutil
import tarfile
import tempfile
import threading
import time
from pathlib import Path
from typing import BinaryIO

from cryptography.fernet import Fernet, InvalidToken


class RepositoryError(Exception):
    pass


class InvalidPackage(RepositoryError):
    pass


class PackageExists(RepositoryError):
    pass


class PackageNotFound(RepositoryError):
    pass


class PackageRepository:
    def __init__(self, root: Path):
        self.root = Path(root)
        self.packages_dir = self.root / "packages"
        self.db_dir = self.root / "database"
        self.db_file = self.db_dir / "packages.db"
        self.key_file = self.root / "db.key"
        self._table: dict[str, dict] = {}
        self._lock = threading.Lock()
        self._fernet: Fernet | None = None

    #On Boot / Unboot

    def load(self) -> None:
        self.packages_dir.mkdir(parents=True, exist_ok=True)
        self.db_dir.mkdir(parents=True, exist_ok=True)
        self._fernet = Fernet(self._load_key())

        if not self.db_file.exists():
            self._table = {}
            return
        try:
            raw = self._fernet.decrypt(self.db_file.read_bytes())
        except InvalidToken as e:
            # Refuse to boot: saving at shutdown would destroy the real table.
            raise RuntimeError("cannot decrypt package database (wrong key or corrupt file)") from e
        self._table = json.loads(raw)

    def save(self) -> None:
        assert self._fernet is not None, "load() must be called first"
        with self._lock:
            blob = self._fernet.encrypt(json.dumps(self._table).encode())
        tmp = self.db_file.with_suffix(".tmp")
        tmp.write_bytes(blob)
        os.chmod(tmp, 0o600)
        os.replace(tmp, self.db_file)

    def _load_key(self) -> bytes:
        env = os.environ.get("PKGSERVER_DB_KEY")
        if env:
            return env.encode()
        if self.key_file.exists():
            return self.key_file.read_bytes().strip()
        key = Fernet.generate_key()
        fd = os.open(self.key_file, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, "wb") as f:
            f.write(key)
        return key

    # Operations

    def add(self, filename: str, version: str, stream: BinaryIO) -> dict:
        safe = Path(filename).name
        if safe in ("", ".", "..") or safe.startswith(".") or not safe.lower().endswith(".tar"):
            raise InvalidPackage("Only .tar files are accepted")

        with self._lock:
            if safe in self._table or (self.packages_dir / safe).exists():
                raise PackageExists("Package already exists")

        with tempfile.TemporaryDirectory(dir=self.packages_dir, prefix=".upload-") as tmp_dir:
            tmp = Path(tmp_dir) / safe
            with open(tmp, "wb") as out:
                shutil.copyfileobj(stream, out, length=1024 * 1024)
            if not tarfile.is_tarfile(tmp):
                raise InvalidPackage("File is not a valid tar archive")

            entry = {
                "name": safe[:-4],
                "version": version,
                "size": tmp.stat().st_size,
                "last_update": int(time.time()),
            }
            with self._lock:
                if safe in self._table:
                    raise PackageExists("Package already exists")
                tmp.rename(self.packages_dir / safe)
                self._table[safe] = entry
        return entry

    def path_of(self, filename: str) -> Path:
        with self._lock:
            if filename not in self._table:
                raise PackageNotFound("Package not found")
        path = self.packages_dir / filename
        if not path.is_file():
            raise PackageNotFound("Package not found")
        return path

    def get(self, filename: str) -> dict:
        with self._lock:
            if filename not in self._table:
                raise PackageNotFound("Package not found")
            return dict(self._table[filename])

    def list(self) -> dict[str, dict]:
        with self._lock:
            return {k: dict(v) for k, v in self._table.items()}
