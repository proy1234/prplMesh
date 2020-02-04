#!/bin/bash

scriptdir="$(cd "${0%/*}"; pwd)"
rootdir="${scriptdir%/*/*/*}"

. "${rootdir}/tools/docker/functions.sh"

OUTPUT_FILE="$rootdir/cppcheck_results.txt"

usage() {
    echo "usage: $(basename "$0") <source> [source]"
    echo "run cpp check on <source> and output number of issues per severity"
}

generate_suppression_file(){
    local suppressed_folders
    suppressed_folders="agent controller framework"
    if ! SUPPRESSION_FILE="$(mktemp)" ; then
        err "Failed to create temporary file, aborting."
    fi
    trap 'rm "$SUPPRESSION_FILE"' EXIT
    for f in $suppressed_folders ; do
        find "$rootdir/$f" -type d | xargs printf '*:%s/*\n' >> "$SUPPRESSION_FILE"
    done
}

run_cppcheck() {
    # --force is needed to check more than 12 ifdefs
    cppcheck --template='{file}:{line}:{column}: {severity}: {message} [{id}]\n{code}' --force --error-exitcode=1 --enable=warning,style,information -q --project="$COMPILATION_DB" --suppressions-list="$SUPPRESSION_FILE" --config-exclude="$rootdir/framework/external/easylogging/" "$@" 2>&1 | tee "$OUTPUT_FILE"
}

colorize_severity() {
    local severity msg
    severity="$1"
    msg="$2"
    case "$severity" in
        error)
            printf '\033[1;31m%s\033[0m\n' "$msg"
            ;;
        warning)
            printf '\033[1;33m%s\033[0m\n' "$msg"
            ;;
        style)
            printf '\033[1;34m%s\033[0m\n' "$msg"
            ;;
        performance)
            printf '\033[1;35m%s\033[0m\n' "$msg"
            ;;
        portability)
            printf '\033[1;36m%s\033[0m\n' "$msg"
            ;;
        ok)
            printf '\033[1;90m%s\033[0m\n' "$msg"
            ;;

        *)
            echo unknown severity "$severity"
            exit 1
            ;;
    esac
}

output_results() {
    local severities status
    severities="error warning style performance portability"
    status=0
    for s in $severities; do
        nb_issues="$(grep -Ec "^[^ ]+ $s:" "$OUTPUT_FILE")"
        if [ $nb_issues -gt 0 ] ; then
            colorize_severity "$s" "$nb_issues issues with severity $s"
            case "$s" in
                # report the following severities in exit code:
                error|warning)
                    status=$((status + 1))
                    ;;
                # ignore all others:
                *);;
            esac
        else
            colorize_severity "ok" "$nb_issues issues with severity $s"
        fi
    done
    return $status
}

main() {
    local build_cmd

    if ! command -v cppcheck > /dev/null ; then
        err "This script requires cppcheck! Please install it and try again."
        exit 1
    fi

    COMPILATION_DB="$rootdir/build/compile_commands.json"

    if [ ! -f "$COMPILATION_DB" ] ; then
        echo "compilation database does not exist, it will be created with default build arguments"
        build_cmd="cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$rootdir/build/install -H. -B$rootdir/build -G Ninja"
        echo "build_cmd: $build_cmd"
        if ! $build_cmd ; then
            err "Error: failed to generate the compilation database."
            exit 1
        fi
    fi

    generate_suppression_file
    run_cppcheck "$@"
    output_results
}

main "$@"
