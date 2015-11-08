# Copyright (c) 2015, Louis Opter <kalessin@kalessin.fr>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

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

lightsc_get_pipe() {
    local pipe=${LIGHTSD_COMMAND_PIPE:-`lightsd --rundir`/pipe}
    if [ ! -p $pipe ] ; then
        echo >&2 "$pipe cannot be found, is lightsd running?"
        exit 1
    fi
    echo $pipe
}

# Can be used to build batch request:
#
# tee `lightsc_get_pipe` <<EOF
# [
#     $(lightsc_make_request power_on ${*:-'"#br"'}),
#     $(lightsc_make_request set_light_from_hsbk ${*:-'"#tower"'} 37.469443 1.0 0.05 3500 600),
#     $(lightsc_make_request set_light_from_hsbk ${*:-'["candle","fugu"]'} 47.469443 0.2 0.05 3500 600)
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

    lightsc_make_request $* | tee `lightsc_get_pipe` | _lightsc_jq
}

# vim: set expandtab ft=sh sts=4 sw=4:
