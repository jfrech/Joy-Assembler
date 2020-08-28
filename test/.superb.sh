#! /bin/sh
# run all .asm files in programs/ and define their memory dump as pristine

root="$(dirname "$(realpath "$0")")"
make -C "$root/.." || exit 1

pristine="$root/pristine"
programs="$root/programs"

find "$root/programs" -mindepth 1 -maxdepth 1 -type f | while read prg; do
    pristineMemoryDump="$pristine/$(realpath "$prg" --relative-to="$programs").dmp"
    printf 'running: %s ...\n' "$prg"
    "$root/../joy-assembler" "$prg" memory-dump > "$pristineMemoryDump"
done
