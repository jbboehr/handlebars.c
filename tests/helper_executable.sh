#!/bin/sh

set -eu

if [ -n "${HELPER_MARKER_FILE-}" ]; then
    printf '%s\n' invoked > "$HELPER_MARKER_FILE"
fi

if IFS= read -r request; then
    if [ -n "${HELPER_REQUEST_FILE-}" ]; then
        printf '%s\n' "$request" > "$HELPER_REQUEST_FILE"
    fi
    case "$request" in
        *'"helper":"jsonSafe"'*)
            printf '%s' '{"protocol":1,"ok":true,"value":"<strong>safe</strong>","safe":true}'
            ;;
        *'"helper":"jsonEscaped"'*)
            printf '%s' '{"protocol":1,"ok":true,"value":"<strong>escaped</strong>","safe":false}'
            ;;
        *'"helper":"jsonArray"'*)
            printf '%s' '{"protocol":1,"ok":true,"value":[1,"x",true],"safe":false}'
            ;;
        *'"helper":"jsonVersion"'*)
            printf '%s' '{"protocol":2,"ok":true,"value":"wrong version","safe":false}'
            ;;
        *'"helper":"jsonUnsafe"'*)
            printf '%s' '{"protocol":1,"ok":true,"value":[1],"safe":true}'
            ;;
        *'"helper":"jsonError"'*)
            printf '%s' '{"protocol":1,"ok":false,"error":"fixture rejected the request"}'
            ;;
        *'"helper":"jsonInvalid"'*)
            printf '%s' '{"protocol":1,"ok":true,"value":"broken"} trailing'
            ;;
        *'"helper":"jsonNaN"'*)
            printf '%s' '{"protocol":1,"ok":true,"value":NaN,"safe":false}'
            ;;
        *'"helper":"jsonInfinity"'*)
            printf '%s' '{"protocol":1,"ok":true,"value":Infinity,"safe":false}'
            ;;
        *'"helper":"jsonComment"'*)
            printf '%s' '/* comment */ {"protocol":1,"ok":true,"value":"comment","safe":false}'
            ;;
        *'"helper":"jsonVerticalTab"'*)
            printf '%b' '\013{"protocol":1,"ok":true,"value":"vertical tab","safe":false}'
            ;;
        *'"helper":"jsonFormFeed"'*)
            printf '%b' '{"protocol":1,"ok":true,"value":"form feed","safe":false}\014'
            ;;
        *'"helper":"jsonInvalidUtf8"'*)
            printf '%b' '{"protocol":1,"ok":true,"value":"\377","safe":false}'
            ;;
        *'"helper":"jsonUnpairedSurrogate"'*)
            printf '%s' '{"protocol":1,"ok":true,"value":"\uD800","safe":false}'
            ;;
        *'"helper":"jsonUnicode"'*)
            printf '%b' '{"protocol":1,"ok":true,"value":"caf\303\251 \\uD83D\\uDE00","safe":false}'
            ;;
        *'"helper":"jsonInvalidOutbound"'*)
            printf '%s' '{"protocol":1,"ok":true,"value":"invoked","safe":false}'
            ;;
        *'"helper":"jsonInvalidDescendant"'*)
            sleep 30 </dev/null >/dev/null 2>&1 &
            printf '%s\n' "$!" > "${HELPER_PID_FILE:?}"
            printf '%s' '{"protocol":1,"ok":true,"value":"broken"} trailing'
            ;;
        *'"helper":"jsonRequest"'*)
            case "$request" in
                *'"protocol":1'*'"args":["a",2,true,null,[3]]'*'"hash":{"key":"v"}'*'"scope":{"items":[3]}'*'"data":null'*)
                    printf '%s' '{"protocol":1,"ok":true,"value":"request-ok","safe":false,"ignored":1}'
                    ;;
                *)
                    printf '%s' '{"protocol":1,"ok":false,"error":"unexpected request shape"}'
                    ;;
            esac
            ;;
        *'"helper":"jsonNested"'*)
            case "$request" in
                *'"args":[{"items":[3]}]'*)
                    printf '%s' '{"protocol":1,"ok":true,"value":"nested-ok","safe":false}'
                    ;;
                *)
                    printf '%s' '{"protocol":1,"ok":false,"error":"unexpected nested request shape"}'
                    ;;
            esac
            ;;
        *)
            printf '%s' '{"protocol":1,"ok":false,"error":"unknown JSON fixture helper"}'
            ;;
    esac
    exit 0
fi

case "${1-}" in
    fail)
        printf '%s\n' 'fixture stderr' >&2
        exit 7
        ;;
    fail-descendant)
        sleep 30 </dev/null >/dev/null 2>&1 &
        printf '%s\n' "$!" > "${HELPER_PID_FILE:?}"
        exit 7
        ;;
    success-descendant)
        sleep 30 </dev/null >/dev/null 2>&1 &
        printf '%s\n' "$!" > "${HELPER_PID_FILE:?}"
        printf '%s' complete
        ;;
    sleep)
        sleep 2
        printf '%s' late
        ;;
    close-stdout)
        parent_pid=$PPID
        exec 1>&-
        (
            sleep 0.10
            kill -CONT "$parent_pid"
        ) </dev/null >/dev/null 2>&1 &
        kill -STOP "$parent_pid"
        ;;
    close-stdout-late)
        exec 1>&-
        sleep 2
        ;;
    stop-parent-then-complete)
        parent_pid=$PPID
        (
            sleep 0.10
            kill -CONT "$parent_pid"
        ) </dev/null >/dev/null 2>&1 &
        kill -STOP "$parent_pid"
        printf '%s' complete
        ;;
    timeout-descendant)
        sleep 30 &
        printf '%s\n' "$!" > "${HELPER_PID_FILE:?}"
        ;;
    stream)
        i=0
        while [ "$i" -lt 32 ]; do
            yes &
            i=$((i + 1))
        done
        wait
        ;;
    large)
        i=0
        while [ "$i" -lt 128 ]; do
            printf x
            i=$((i + 1))
        done
        ;;
    *)
        separator=
        for argument in "$@"; do
            printf '%s%s' "$separator" "$argument"
            separator='|'
        done
        ;;
esac
