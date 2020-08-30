#! /bin/sh
# run all .asm files in programs/ and define their memory dump as pristine

root="$(dirname "$(realpath "$0")")"
make -C "$root/.." || exit 1

pristineHashes="$root/pristine-hashes"
programs="$root/programs"

mkdir -p "pristineHashes"

find "$root/programs" -mindepth 1 -maxdepth 1 -type f | sort | \
while read prg; do
    pristineMemoryDumpHash="$pristineHashes/$(realpath "$prg" --relative-to="$programs").dmp.hsh"
    printf 'running: %s ...\n' "$prg"
    "$root/../joy-assembler" "$prg" memory-dump | sha512sum \
        > "$pristineMemoryDumpHash"
done
