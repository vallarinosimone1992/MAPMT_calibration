#!/usr/bin/env bash

set -euo pipefail

daq_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
default_suite_dir="$(cd "${daq_dir}/.." && pwd)"
suite_dir="${MAPMT_SUITE:-${default_suite_dir}}"
suite_dir="$(cd "${suite_dir}" && pwd)"
export MAPMT_SUITE="${suite_dir}"

config_file="${MAPMT_DAQ_CONFIG:-${daq_dir}/daq.env}"
if [[ -f "${config_file}" ]]; then
  # shellcheck source=/dev/null
  source "${config_file}"
elif [[ -f "${daq_dir}/daq.env.example" ]]; then
  # shellcheck source=/dev/null
  source "${daq_dir}/daq.env.example"
fi

: "${MAPMT_DAQ_EXEDIR:=${CODA:-}/src/rol/Linux_i686_vme/bin}"
: "${MAPMT_DATA_DIR:=${suite_dir}/data}"
: "${MAPMT_PED_DATA_DIR:=${MAPMT_DATA_DIR}/ped}"
: "${MAPMT_TDC_DATA_DIR:=${MAPMT_DATA_DIR}/tdc}"
: "${MAPMT_LOG_DIR:=${MAPMT_DATA_DIR}/logs}"
: "${MAPMT_SETUP_FILE:=setup.txt}"
: "${MAPMT_SCALER_PREFIX:=ssprich_scaler}"
: "${MAPMT_SCALER_OUTFILE:=ssprich_countsCTEST0.txt}"
: "${MAPMT_TDC_RAW_FILE:=ssprich_tdc.bin}"
: "${MAPMT_Q:=0}"
: "${MAPMT_SCALER_DURATION:=2}"
: "${MAPMT_DEFAULT_THRESHOLD:=230}"
: "${MAPMT_DEFAULT_GAIN:=64}"
: "${MAPMT_DEFAULT_SUPPRESS:=0}"

resolve_path() {
  local path="$1"
  if [[ "${path}" = /* ]]; then
    printf '%s\n' "${path}"
  else
    printf '%s\n' "${suite_dir}/${path}"
  fi
}

ensure_output_dirs() {
  MAPMT_DATA_DIR="$(resolve_path "${MAPMT_DATA_DIR}")"
  MAPMT_PED_DATA_DIR="$(resolve_path "${MAPMT_PED_DATA_DIR}")"
  MAPMT_TDC_DATA_DIR="$(resolve_path "${MAPMT_TDC_DATA_DIR}")"
  MAPMT_LOG_DIR="$(resolve_path "${MAPMT_LOG_DIR}")"
  mkdir -p "${MAPMT_PED_DATA_DIR}" "${MAPMT_TDC_DATA_DIR}" "${MAPMT_LOG_DIR}"
}

ssptest_path() {
  local command_name="$1"
  if [[ -n "${MAPMT_DAQ_EXEDIR}" && -x "${MAPMT_DAQ_EXEDIR}/${command_name}" ]]; then
    printf '%s\n' "${MAPMT_DAQ_EXEDIR}/${command_name}"
  elif command -v "${command_name}" >/dev/null 2>&1; then
    command -v "${command_name}"
  else
    printf 'ERROR: cannot find %s. Set MAPMT_DAQ_EXEDIR in %s.\n' \
      "${command_name}" "${config_file}" >&2
    return 127
  fi
}

timestamp() {
  date '+%Y%m%d_%H%M%S'
}

archive_setup_copy() {
  local date_tag="$1"
  if [[ -f "${MAPMT_SETUP_FILE}" ]]; then
    cp -p "${MAPMT_SETUP_FILE}" "${MAPMT_PED_DATA_DIR}/rich_setup_${date_tag}.txt"
  fi
}

configure_if_needed() {
  local threshold="$1"
  local gain="$2"
  local suppress="${3:-${MAPMT_DEFAULT_SUPPRESS}}"
  if [[ ! -f "${MAPMT_SETUP_FILE}" ]]; then
    echo "File ${MAPMT_SETUP_FILE} not found. Running ssptest_ConfigureAll."
    "$(ssptest_path ssptest_ConfigureAll)" "${threshold}" "${gain}" "${suppress}" \
      | tee "${MAPMT_LOG_DIR}/configure_$(timestamp).log"
  fi
}

clean_scaler_fragments() {
  rm -f "${MAPMT_SCALER_PREFIX}"_???.txt
}
