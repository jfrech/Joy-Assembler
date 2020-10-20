#! /bin/sh
# test all .asm files in programs/

root="$(dirname "$(realpath "$0")")"
make -C "$root/.." || exit 1

pristineHashes="$root/pristine-hashes"
programs="$root/programs"
mkdir -p "$pristineHashes" "$programs"

tmp="$root/.tmp.dmp"

find "$root/programs" -mindepth 1 -maxdepth 1 -type f | sort | \
while read prg; do
    printf 'testing: %s\n' "$prg"
    _mirrored="$(realpath "$prg" --relative-to="$programs").dmp.hsh"
    pristineMemoryDumpHash="$pristineHashes/$(realpath "$prg" --relative-to="$programs").dmp.hsh"
    printf '    looking for pristine memory dump hash: %s\n' \
        "$pristineMemoryDumpHash"
    [ ! -f "$pristineMemoryDumpHash" ] \
        && printf '    \33[38;5;124m[ERR]\33[0m could not find pristine ' \
        && printf 'memory dump hash\n' && exit 1
    printf '    executing: %s ...\n' "$prg"
    "$root/../JoyAssembler" "$prg" memory-dump | sha512sum > "$tmp"
    printf '    comparing to pristine memory dump hash\n'
    ! cmp 2>/dev/null "$pristineMemoryDumpHash" "$tmp" \
        && printf '        \33[38;5;124m[ERR]\33[0m memory dump hash ' \
        && printf 'mismatch\n      Please note that tests named "test-r-"* ' \
        && printf 'may perform platform-dependend.\n      To recalibrate all ' \
        && printf 'pristine hashes, ' \
        && printf '\33[3mfundamentally altering all tests\33[0m, you may run ' \
        && printf '\n          %% ./test/.superb.sh\n' && exit 1

    printf '        \33[38;5;154m[SUC]\33[0m memory dump hash match\n'
done || exit 1

rm "$tmp"
printf '\n\33[38;5;154m[SUC]\33[0m every test has passed\n' && exit 0
