#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/daq_common.sh"

start="${1:-${MAPMT_DARK_START:-100}}"
stop="${2:-${MAPMT_DARK_STOP:-700}}"
gain="${3:-${MAPMT_DARK_GAIN:-0}}"

"${BASH_SOURCE[0]%/*}/thr_scan.sh" "${start}" "${stop}" "${gain}" "dark"
