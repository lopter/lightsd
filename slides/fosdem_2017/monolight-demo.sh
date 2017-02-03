#!/bin/sh

check_process() {
  local name=$1

  /bin/echo -n "checking that ${name} is running: "
  pgrep ${name} 2>&1 >/dev/null && echo ok || { echo "not running"; exit 1; }
}

check_venv() {
  local name=$1

  /bin/echo -n "checking that the virtualenv ${name} is active: "
  echo ${VIRTUAL_ENV} | grep ${name} 2>&1 >/dev/null && echo ok || { echo nope; exit 1; }
}

check_venv monolight

check_process serialoscd

cat <<EOF
### monolight demo ###

--> Controls
--> UI feedback
--> monolight layer definitions

######################
EOF

exec monolight $*
