#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/daq_common.sh"
ensure_output_dirs

threshold="${1:-${MAPMT_DEFAULT_THRESHOLD}}"
gain="${2:-${MAPMT_DEFAULT_GAIN}}"
suppress="${3:-${MAPMT_DEFAULT_SUPPRESS}}"
date_tag="$(timestamp)"

echo "Running ssptest_ConfigureAll threshold=${threshold} gain=${gain} suppress=${suppress}"
"$(ssptest_path ssptest_ConfigureAll)" "${threshold}" "${gain}" "${suppress}" \
  | tee "${MAPMT_LOG_DIR}/configure_${date_tag}.log"

if [[ -f "${MAPMT_SETUP_FILE}" ]]; then
  cp -p "${MAPMT_SETUP_FILE}" "${MAPMT_LOG_DIR}/setup_${date_tag}.txt"
fi
