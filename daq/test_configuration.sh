#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/daq_common.sh"
ensure_output_dirs

ntries="${1:-1000}"
threshold="${2:-${MAPMT_DEFAULT_THRESHOLD}}"
gain="${3:-${MAPMT_DEFAULT_GAIN}}"
suppress="${4:-${MAPMT_DEFAULT_SUPPRESS}}"
date_tag="$(timestamp)"
out="${MAPMT_LOG_DIR}/test_maroc_discovery_${date_tag}.txt"

configure_cmd="$(ssptest_path ssptest_ConfigureAll)"
for ((i = 1; i <= ntries; i += 1)); do
  echo ">>> Configure test ${i}/${ntries}"
  "${configure_cmd}" "${threshold}" "${gain}" "${suppress}" | grep repetition >> "${out}" || true
done

echo "Stored configuration test log: ${out}"
