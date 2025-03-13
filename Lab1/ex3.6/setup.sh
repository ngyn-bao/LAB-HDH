#!/bin/bash

BKU_PATH="/usr/local/bin/bku"
DEPENDENCIES=("diff" "cron")

checkDependencies(){
    echo Checking dependencies...
    missingDeps=()
    for dep in "${DEPENDENCIES[@]}"; do
        if ! command -v "$dep" &>/dev/null; then
            missingDeps+=("$dep")
        fi
    done

    for missingDep in "${missingDeps[@]}"; do
    if [ "$missingDep" -ne 0 ]; then
        echo Missing dependencies: "$missingDep"
        echo Installing missing dependencies...
        if command -v apt &>/dev/null; then
            sudo apt update && sudo apt install -y "$missingDep"
        elif command -v yum &>/dev/null; then
            sudo yum install -y "$missingDep"
        else
            echo Error: Failed to install packages. Please check your package manager or install them manually.
            exit 1
        fi  
    fi
    done

    echo All dependencies installed.
}

install(){
    checkDependencies
    echo Installing BKU...
    sudo cp bku.sh "$BKU_PATH"
    sudo chmod +x "$BKU_PATH"
    echo BKU installed to "$BKU_PATH"
}

uninstall(){
    checkDependencies
    if [ ! -f "$BKU_PATH" ]; then
        echo Error: BKU is not installed in "$BKU_PATH"
        echo Nothing to uninstall.
        exit 1
    fi
    echo Removing BKU from "$BKU_PATH"
    sudo rm "$BKU_PATH"
    echo Removing scheduled backups...
    crontab -l | grep -v "$BKU_PATH" | crontab - 
    echo BKU successfully uninstalled.
}

case "$1" in
    --install) install ;;
    --uninstall) uninstall ;;
    *) echo "Usage: ./setup.sh {--install|--uninstall}" ;;
esac
