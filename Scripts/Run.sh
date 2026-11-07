#!/usr/bin/env bash
# Scripts/Run.sh
# Inker Project
# Copyright (C) 2026 Inker Developer kljj04

set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"

MAKEFILE="${BASE_DIR}/Makefiles/Makefile"
TARGET="${BASE_DIR}/Build/inker"
DAEMON_TARGET="${BASE_DIR}/Build/inkerd"

INSTALL_DIR="${HOME}/.local/bin"
INSTALL_TARGET="${INSTALL_DIR}/inker"
DAEMON_INSTALL_TARGET="${INSTALL_DIR}/inkerd"

JOBS="$(nproc)"

if [[ -t 1 && -z "${NO_COLOR:-}" && "${TERM:-}" != "dumb" ]]; then
    RESET=$'\033[0m'
    BOLD=$'\033[1m'
    BLUE=$'\033[38;2;100;180;255m'
    GREEN=$'\033[38;2;120;220;140m'
    YELLOW=$'\033[38;2;255;195;110m'
    RED=$'\033[38;2;255;110;110m'
    GRAY=$'\033[38;2;145;150;165m'
else
    RESET=""
    BOLD=""
    BLUE=""
    GREEN=""
    YELLOW=""
    RED=""
    GRAY=""
fi

Info()
{
    printf '%s[INFO]%s %s\n' "${BLUE}" "${RESET}" "$*"
}

Success()
{
    printf '%s[ OK ]%s %s\n' "${GREEN}" "${RESET}" "$*"
}

Warning()
{
    printf '%s[WARN]%s %s\n' "${YELLOW}" "${RESET}" "$*" >&2
}

Error()
{
    printf '%s[FAIL]%s %s\n' "${RED}" "${RESET}" "$*" >&2
}

Header()
{
    printf '\n%s%sInker Build & Install%s\n' "${BOLD}" "${BLUE}" "${RESET}"
    printf '%s━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━%s\n\n' "${GRAY}" "${RESET}"
}

ShowUsage()
{
    cat <<'EOF'
Usage: ./Scripts/Run.sh [OPTION]

Options:
  -b, --build       Build Inker and Inkerd
  -i, --install     Build and install Inker and Inkerd (default)
  -c, --clean       Remove build artifacts
  -R, --rebuild     Clean, build and install Inker
  -f, --format      Format C++ source files
  -u, --uninstall   Uninstall Inker
  -h, --help        Show this help

Examples:
  ./Scripts/Run.sh
  ./Scripts/Run.sh --build
  ./Scripts/Run.sh --install
  ./Scripts/Run.sh --rebuild
  ./Scripts/Run.sh --uninstall
EOF
}

GetTime()
{
    date +%s%N
}

PrintElapsed()
{
    local Start="$1"
    local End="$2"
    local ElapsedMs=$(((End - Start) / 1000000))

    printf '%s%d.%03ds%s' "${GRAY}" "$((ElapsedMs / 1000))" "$((ElapsedMs % 1000))" "${RESET}"
}

Build()
{
    local Start
    local End

    Info "Building Inker..."

    Start="$(GetTime)"

    make --no-print-directory -s -C "${BASE_DIR}" -f "${MAKEFILE}" -j"${JOBS}" All

    End="$(GetTime)"

    printf '%s[ OK ]%s Build completed (' "${GREEN}" "${RESET}"
    PrintElapsed "${Start}" "${End}"
    printf ')\n'
}

Install()
{
    Info "Installing Inker..."

    if [[ ! -f "${TARGET}" || ! -f "${DAEMON_TARGET}" ]]; then
        Error "Build output not found: ${TARGET}"
        return 1
    fi

    mkdir -p "${INSTALL_DIR}"
    install -m 755 "${TARGET}" "${INSTALL_TARGET}"
    install -m 755 "${DAEMON_TARGET}" "${DAEMON_INSTALL_TARGET}"

    Success "Installed to ${INSTALL_TARGET} and ${DAEMON_INSTALL_TARGET}"

    if [[ ":${PATH}:" != *":${INSTALL_DIR}:"* ]]; then
        Warning "${INSTALL_DIR} is not in PATH."
        printf 'Add this line to your shell configuration:\n'
        printf '  export PATH="$HOME/.local/bin:$PATH"\n'
    else
        Success "Complete."
    fi
}

Clean()
{
    Info "Cleaning build artifacts..."

    make --no-print-directory -s -C "${BASE_DIR}" -f "${MAKEFILE}" Clean

    Success "Clean completed."
}

Format()
{
    Info "Formatting source files..."

    make --no-print-directory -s -C "${BASE_DIR}" -f "${MAKEFILE}" Format

    Success "Formatting completed."
}

Uninstall()
{
    if [[ ! -e "${INSTALL_TARGET}" && ! -e "${DAEMON_INSTALL_TARGET}" ]]; then
        Warning "Inker is not installed."
        return 0
    fi

    Info "Uninstalling Inker..."

    rm -f -- "${INSTALL_TARGET}" "${DAEMON_INSTALL_TARGET}"

    Success "Inker uninstalled."
}

OnError()
{
    local Status=$?

    trap - ERR

    Error "Operation failed (exit ${Status})."
    exit "${Status}"
}

trap OnError ERR

Mode="${1:---install}"

if (( $# > 1 )); then
    Error "Too many arguments."
    ShowUsage
    exit 2
fi

if [[ "${Mode}" == "-h" || "${Mode}" == "--help" ]]; then
    ShowUsage
    exit 0
fi

Header

case "${Mode}" in
    -b|--build)
        Build
        ;;
    -i|--install)
        Build
        Install
        ;;
    -c|--clean)
        Clean
        ;;
    -R|--rebuild)
        Clean
        Build
        Install
        ;;
    -f|--format)
        Format
        ;;
    -u|--uninstall)
        Uninstall
        ;;
    *)
        Error "Unknown option: ${Mode}"
        ShowUsage
        exit 2
        ;;
esac