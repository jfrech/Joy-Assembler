#! /bin/sh

readme="$(dirname "$(realpath "$0")")/README.md"

isoDate () {
    date +'%Y-%m-%dT%H:%M:%S%:z'
}
buildInfo () {
    sed --in-place \
        "$(printf 's!^\*\*Build: .*$!**Build: %s** (%s)!' "$1" "$(isoDate)")" \
        "$readme"
}

case "$1" in
    'passing') buildInfo "$(printf '\xf0\x9f\x9f\xa9 passing')" ;;
    'failing') buildInfo "$(printf '\xf0\x9f\x9f\xa5 failing')" ;;
    *) printf 'unknown build status: "%s"\n' "$1" >&2 && exit 1 ;;
esac
