#! /bin/sh
# test all .asm files in programs/

root="$(dirname "$(realpath "$0")")"
make -C "$root/.." || exit 1

pristine="$root/pristine"
programs="$root/programs"
tmp="$root/.tmp.dmp"


isoDate () {
    date +'%Y-%m-%dT%H:%M:%SZ'
}
buildInfo () {
    sed --in-place "$(printf 's!^\*\*Build: .*$!**Build: %s** (%s)!' "$1" "$(isoDate)")" "$root/../README.md"
}
buildInfo "$(printf '\xf0\x9f\x9f\xa5 failing')"

find "$root/programs" -mindepth 1 -maxdepth 1 -type f | while read prg; do
    printf 'testing: %s\n' "$prg"
    pristineMemoryDump="$pristine/$(realpath "$prg" --relative-to="$programs").dmp"
    printf '    looking for pristine memory dump: %s\n' "$pristineMemoryDump"
    [ ! -f "$pristineMemoryDump" ] \
        && printf '    \33[38;5;124m[ERR]\33[0m could not find pristine memory dump\n' && exit 1
    printf '    executing: %s ...\n' "$prg"
    "$root/../joy-assembler" "$prg" memory-dump > "$tmp"
    printf '    comparing to pristine memory dump\n'
    ! cmp 2>/dev/null "$pristineMemoryDump" "$tmp" \
        && printf '        \33[38;5;124m[ERR]\33[0m memory dump mismatch\n' && exit 1

    printf '        \33[38;5;154m[SUC]\33[0m memory dump match\n'
done || exit 1

buildInfo "$(printf '\xf0\x9f\x9f\xa9 passing')"

rm "$tmp"
