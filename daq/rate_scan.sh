#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/daq_common.sh"
ensure_output_dirs

nruns="${1:-${MAPMT_RATE_RUNS:-100}}"
gain="${2:-${MAPMT_RATE_GAIN:-0}}"
fixed_threshold="${3:-${MAPMT_RATE_FIXED_THRESHOLD:-230}}"

configure_if_needed "${fixed_threshold}" "${gain}" "${MAPMT_DEFAULT_SUPPRESS}"

rm -f "${MAPMT_SCALER_OUTFILE}"
clean_scaler_fragments

scaler_cmd="$(ssptest_path ssptest_ScalerAll)"
for ((run = 1; run <= nruns; run += 1)); do
  echo ">>> Counting run ${run}/${nruns}, threshold ${fixed_threshold}, gain ${gain}"
  "${scaler_cmd}" "${fixed_threshold}" "${gain}" "${MAPMT_Q}" "${MAPMT_SCALER_DURATION}"
  cat "${MAPMT_SCALER_PREFIX}"_???.txt >> "${MAPMT_SCALER_OUTFILE}"
  clean_scaler_fragments
done

date_tag="$(timestamp)"
out="${MAPMT_PED_DATA_DIR}/rich_rate_${date_tag}.txt"
mv "${MAPMT_SCALER_OUTFILE}" "${out}"
archive_setup_copy "${date_tag}"
echo "Stored fixed-threshold scaler file: ${out}"
