#!/usr/bin/env bash

command -v uncrustify > /dev/null || (echo "uncrustify is not installed on the system. Skipping. See: https://github.com/uncrustify/uncrustify or download the binary for your system" && exit 1)

UNCRUSTIFY_ACTION="--check"
UNCRUSTIFY_ARGS=""

while test $# -gt 0; do
  case "$1" in
    -h|--help)
      echo "uncrustify.sh: Runs uncrustify on files under 'src' and 'include'. Looks for *.cpp and *.hpp files"
      echo " "
      echo "Warning: This is a destructive command. It's recommended to commit current work before running this command."
      echo "         It could alter the meaning of the code."
      echo "uncrustify.sh [options]"
      echo " "
      echo "options:"
      echo "-h, --help                show brief help"
      echo "-u, --update              Updates the files that have any difference"
      echo "-q, --quiet               Supress some output"
      exit 0
      ;;
    -u|--update)
      shift
      UNCRUSTIFY_ACTION="--no-backup --if-changed"
      ;;
    -q|--quiet)
      shift
      UNCRUSTIFY_ARGS="${UNCRUSTIFY_ARGS} -q"
      ;;
    *)
      break
      ;;
  esac
done

find src include -type f \( -iname \*.hpp -o -iname \*.cpp \) -print0 | xargs -0 uncrustify -c .config/uncrustify.config ${UNCRUSTIFY_ACTION} ${UNCRUSTIFY_ARGS}
