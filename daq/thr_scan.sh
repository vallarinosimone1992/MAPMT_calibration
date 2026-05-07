#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/daq_common.sh"
ensure_output_dirs

if [[ $# -lt 3 || $# -gt 4 ]]; then
  echo "USAGE: $0 <start> <stop> <gain> [label]" >&2
  echo "  gain: 1..255, or 0 to use the loaded gain map" >&2
  exit 2
fi

start="$1"
stop="$2"
gain="$3"
label="${4:-pedestal}"

configure_if_needed 0 "${gain}" "${MAPMT_DEFAULT_SUPPRESS}"

rm -f "${MAPMT_SCALER_OUTFILE}"
clean_scaler_fragments

scaler_cmd="$(ssptest_path ssptest_ScalerAll)"
for ((thr = start; thr <= stop; thr += 1)); do
  echo ">>> Threshold ${thr}, gain ${gain}, duration ${MAPMT_SCALER_DURATION}s"
  "${scaler_cmd}" "${thr}" "${gain}" "${MAPMT_Q}" "${MAPMT_SCALER_DURATION}"
  cat "${MAPMT_SCALER_PREFIX}"_???.txt >> "${MAPMT_SCALER_OUTFILE}"
  clean_scaler_fragments
done

date_tag="$(timestamp)"
out="${MAPMT_PED_DATA_DIR}/rich_${label}_${date_tag}.txt"
mv "${MAPMT_SCALER_OUTFILE}" "${out}"
archive_setup_copy "${date_tag}"
echo "Stored scaler calibration file: ${out}"
