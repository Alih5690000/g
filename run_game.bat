@echo off
setlocal

REM === Название архива Python и папки ===
set PY_VER=3.12.7
set PY_ZIP=python-%PY_VER%-embed-amd64.zip
set PY_URL=https://www.python.org/ftp/python/%PY_VER%/%PY_ZIP%
set PY_DIR=%~dp0python

REM === Проверяем, установлен ли Python ===
if exist "%PY_DIR%\python.exe" (
    echo [OK] Python уже есть.
) else (
    echo [INFO] Portable Python не найден. Скачиваем %PY_ZIP% ...
    powershell -Command "Invoke-WebRequest '%PY_URL%' -OutFile '%PY_ZIP%'"
    if errorlevel 1 (
        echo [ERR] Не удалось скачать Python. Проверь интернет.
        pause
        exit /b
    )
    echo [INFO] Распаковываем...
    powershell -Command "Expand-Archive '%PY_ZIP%' -DestinationPath '%PY_DIR%' -Force"
    del "%PY_ZIP%"
    echo [OK] Python распакован.
)

REM === Запускаем локальный сервер ===
echo [INFO] Запуск локального сервера на http://localhost:8080/
start "" "http://localhost:8080/index.html"
pushd "%~dp0"
"%PY_DIR%\python.exe" -m http.server 8080
popd

pause
