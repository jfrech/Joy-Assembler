#! /bin/sh
# test all .asm files in programs/

root="$(dirname "$(realpath "$0")")"
make -C "$root/.." || exit 1

pristine="$root/pristine"
programs="$root/programs"
tmp="$root/.tmp.dmp"

find "$root/programs" -mindepth 1 -maxdepth 1 -type f | while read prg; do
    printf 'testing: %s\n' "$prg"
    pristineMemoryDump="$pristine/$(realpath "$prg" --relative-to="$programs").dmp"
    printf '    looking for pristine memory dump: %s\n' "$pristineMemoryDump"
    [ ! -f "$pristineMemoryDump" ] \
        && printf '    \33[38;5;124m[ERR]\33[0m could not find pristine memory dump\n' && continue
    printf '    executing: %s ...\n' "$prg"
    "$root/../joy-assembler" "$prg" memory-dump > "$tmp"
    printf '    comparing to pristine memory dump\n'
    ! cmp 2>/dev/null "$pristineMemoryDump" "$tmp" \
        && printf '        \33[38;5;124m[ERR]\33[0m memory dump mismatch\n' && continue

    printf '        \33[38;5;154m[SUC]\33[0m memory dump match\n'
done

rm "$tmp"
