#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/daq_common.sh"

start="${1:-${MAPMT_PEDESTAL_START:-160}}"
stop="${2:-${MAPMT_PEDESTAL_STOP:-220}}"
gain="${3:-${MAPMT_PEDESTAL_GAIN:-0}}"

"${BASH_SOURCE[0]%/*}/thr_scan.sh" "${start}" "${stop}" "${gain}" "pedestal"
