from contextlib import asynccontextmanager
from pathlib import Path

from fastapi import FastAPI, Form, Request, UploadFile, HTTPException
from fastapi.exceptions import RequestValidationError
from fastapi.responses import FileResponse, PlainTextResponse
from starlette.exceptions import HTTPException as StarletteHTTPException

from repository import InvalidPackage, PackageExists, PackageNotFound, PackageRepository

#Constants

REPO_ROOT = (Path(__file__).resolve().parent / ".." / "repo").resolve()

#Handlers

repo = PackageRepository(REPO_ROOT)


@asynccontextmanager
async def lifespan(app: FastAPI):
    repo.load()   # boot: encrypted table -> dict
    yield
    repo.save()   # shutdown: dict -> encrypted table


#App Declaration

app = FastAPI(lifespan=lifespan, default_response_class=PlainTextResponse, docs_url=None, redoc_url=None, openapi_url=None)


@app.exception_handler(StarletteHTTPException)
async def http_error(request: Request, exc: StarletteHTTPException):
    return PlainTextResponse(f"error: {exc.detail}\n", status_code=exc.status_code, headers=exc.headers)


@app.exception_handler(RequestValidationError)
async def validation_error(request: Request, exc: RequestValidationError):
    lines = [f"{'.'.join(str(x) for x in e['loc'])}: {e['msg']}" for e in exc.errors()]
    return PlainTextResponse("error: invalid request\n" + "\n".join(lines) + "\n", status_code=422)

#EndPoints

@app.post("/uploadpackage/")
def upload_file(file: UploadFile, version: str = Form("unknown")):
    try:
        entry = repo.add(file.filename or "", version, file.file)
    except InvalidPackage as e:
        raise HTTPException(status_code=400, detail=str(e))
    except PackageExists as e:
        raise HTTPException(status_code=409, detail=str(e))
    return PlainTextResponse(
        f"uploaded {entry['name']} {entry['version']} ({entry['size']} bytes)\n", status_code=201
    )


@app.get("/packages/")
def list_packages():
    rows = [
        f"{e['name']}\t{e['version']}\t{e['size']}\t{e['last_update']}"
        for e in repo.list().values()
    ]
    return PlainTextResponse("\n".join(rows) + ("\n" if rows else ""))


@app.get("/packages/{name}")
def download_package(name: str):
    try:
        path = repo.path_of(name)
    except PackageNotFound as e:
        raise HTTPException(status_code=404, detail=str(e))
    return FileResponse(path, media_type="application/x-tar", filename=name)
