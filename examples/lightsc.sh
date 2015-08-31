#!/bin/sh

# Here is an example script that dims bulbs to a warm orange:

# #!/bin/sh
#
# # Optional (default value: /run/lightsd.cmd):
# COMMAND_PIPE=/foo/bar/lightsd.cmd
#
# . /usr/lib/lightsd/lightsc.sh
#
# lightsc set_light_from_hsbk ${*:-'"*"'} 37.469443 1.0 0.05 3500 600

# Here is how you could use it:
#
# - dim all the bulbs: orange
# - dim the bulb named kitchen: orange '"kitchen"'
# - dim the bulb named kitchen and the bulbs tagged bedroom:
#   orange '["kitchen", "#bedroom"]'
#
# You can also load this file directly in your shell rc configuration file.
#
# NOTE: Keep in mind that arguments must be JSON, you will have to enclose
#       tags and labels into double quotes '"likethis"'. Also keep in mind
#       that the pipe is write-only you cannot read any result back.

_lightsc_b64e() {
    if type base64 >/dev/null 2>&1 ; then
        base64
    elif type b64encode >/dev/null 2>&1 ; then
        b64encode
    else
        cat >/dev/null
        echo null
    fi
}

_lightsc_gen_request_id() {
    if type dd >/dev/null 2>&1 ; then
        printf '"%s"' `dd if=/dev/urandom bs=8 count=1 2>&- | _lightsc_b64e`
    else
        echo null
    fi
}

_lightsc_jq() {
    if type jq >/dev/null 2>&1 ; then
        jq .
    else
        cat
    fi
}

_lightsc_get_pipe() {
    local pipe=${COMMAND_PIPE:-/run/lightsd.cmd}
    if [ ! -p $pipe ] ; then
        echo >&2 "$pipe cannot be found, is lightsd running?"
        exit 1
    fi
    echo $pipe
}

# Can be used to build batch request:
#
# tee $COMMAND_PIPE <<EOF
# [
#     $(lightsc_make_request set_light_from_hsbk ${*:-'"#tower"'} 37.469443 1.0 0.05 3500 600),
#     $(lightsc_make_request set_light_from_hsbk ${*:-'["candle","fugu"]'} 47.469443 0.2 0.05 3500 600),
#     $(lightsc_make_request power_on ${*:-'"#br"'})
# ]
# EOF
lightsc_make_request() {
    if [ $# -lt 2 ] ; then
        echo >&2 "Usage: $0 METHOD PARAMS ..."
        return 1
    fi

    local method=$1 ; shift
    local params=$1 ; shift
    for target in $* ; do
        params=$params,$target
    done

    cat <<EOF
{
  "jsonrpc": "2.0",
  "method": "$method",
  "params": [$params],
  "id": `_lightsc_gen_request_id`
}
EOF
}

lightsc() {
    if [ $# -lt 2 ] ; then
        echo >&2 "Usage: $0 METHOD PARAMS ..."
        return 1
    fi

    lightsc_make_request $* | tee `_lightsc_get_pipe` | _lightsc_jq
}
