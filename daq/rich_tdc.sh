#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/daq_common.sh"
ensure_output_dirs

threshold="${1:-${MAPMT_TDC_THRESHOLD:-0}}"
events="${2:-${MAPMT_TDC_EVENTS:-1000000}}"
mode="${3:-${MAPMT_TDC_MODE:-1}}"
repetitions="${4:-${MAPMT_TDC_REPETITIONS:-24}}"
label="${5:-${MAPMT_TDC_LABEL:-tdc}}"
sleep_seconds="${MAPMT_TDC_SLEEP:-3}"

tdc_cmd="$(ssptest_path ssptest_TDCAll)"
for ((i = 1; i <= repetitions; i += 1)); do
  echo ">>> TDC run ${i}/${repetitions}: threshold=${threshold} events=${events} mode=${mode}"
  "${tdc_cmd}" "${threshold}" "${events}" "${mode}"
  date_tag="$(timestamp)"
  out="${MAPMT_TDC_DATA_DIR}/ssprich_tdc_${label}_${date_tag}_${i}.bin"
  mv "${MAPMT_TDC_RAW_FILE}" "${out}"
  echo "Stored TDC raw file: ${out}"
  sleep "${sleep_seconds}"
done
