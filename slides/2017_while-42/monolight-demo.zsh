#!/usr/bin/env zsh

PY3_BIN="${$(command -v python3):?"Couldn't find python3"}"

VIRTUALENVWRAPPER_PYTHON="${PY3_BIN}"
. "$(command -v virtualenvwrapper.sh)"

check_process() {
  local name=$1

  printf "checking that %s is running: " "${name}"
  pgrep ${name} 2>&1 >/dev/null && echo ok || { echo "not running"; exit 1; }
}

setup_venv() {
  local name=$1

  [ "$(lsvirtualenv | grep -c "^${name}$")" -eq 1 ] || {
    mkvirtualenv -qp "${PY3_BIN}" "${name}";
    pip install -q \
        ~/projs/lightsd/clients/python/lightsc \
        ~/projs/lightsd/apps/monolight/;
  }

  [ "${name}" = "${VIRTUAL_ENV}" ] || { workon "${name}"; }

  :
}

setup_venv monolight

check_process serialoscd

cat <<EOF
### monolight demo ###

--> Controls
--> UI feedback
--> monolight layer definitions

######################
EOF

exec monolight $*
