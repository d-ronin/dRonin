#!/bin/bash
DUMP_SYMBOLS_TOOL=$1
SOURCE_DIR=$2

#parent_path=$( cd "$(dirname "${BASH_SOURCE}")" ; pwd -P )
#cd "$parent_path"
debugdir=${SOURCE_DIR}_symbols

function moveSymbols()
{
  for symbol_file in $debugdir/*.sym
  do
    file_info=$(head -n1 $symbol_file)
    IFS=' ' read -a splitlist <<< "${file_info}"
    basefilename=${symbol_file:0:${#symbol_file} - 4}
    dest_dir=$basefilename/${splitlist[3]}
    mkdir -p $dest_dir
    mv $symbol_file $dest_dir
    filename=$(basename "$symbol_file")
    echo "$symbol_file -> $dest_dir/$filename"
  done
}

function generateSymbols()
{
  filename=$(basename "$1")
  debugfile="$debugdir/$filename.sym"
  if [ ! -d "${debugdir}" ] ; then
    echo "creating dir ${debugdir}"
    mkdir -p "${debugdir}"
  fi
  echo "Generating symbols for $1, putting them in ${debugfile}"
  ${DUMP_SYMBOLS_TOOL} ${SOURCE_DIR}/${1} > ${debugfile}
  echo "Striping debug information from $1"
  strip --strip-debug --strip-unneeded "${SOURCE_DIR}/${1}"
}

interesting_files=$(cd ${SOURCE_DIR} && find . -executable -type f -not -path "./share/*" )
for file in $interesting_files; do
    generateSymbols $file
done
moveSymbols
