#!/bin/bash
DUMP_SYMBOLS_TOOL=$1
SOURCE_DIR=$2

function generateSymbols()
{
  FILE_NO_EXT=$(echo "$1" | cut -f 2 -d '.')
  symbol_file=${SOURCE_DIR}_symbols/${FILE_NO_EXT}.sym
  ${DUMP_SYMBOLS_TOOL} "$1" > ${symbol_file}
  echo "Generating symbols for $1"
}

function moveSymbols()
{
  symbol_file=${SOURCE_DIR}_symbols/"$1"
  file_info=$(head -n1 $symbol_file)
  IFS=' ' read -a splitlist <<< "${file_info}"
  dest_dir=${SOURCE_DIR}_symbols/${splitlist[4]}/${splitlist[3]}/
  echo "Moving $symbol_file -> $dest_dir"
  mkdir -p $dest_dir
  mv $symbol_file $dest_dir
}

echo "Copying pdb files to symbols directory $SOURCE_DIR_symbols"
cd ${SOURCE_DIR} && find . -name '*.pdb' -exec install -D {} ${SOURCE_DIR}_symbols/{} \;
interesting_files=$(cd ${SOURCE_DIR}_symbols && find . -name '*.pdb' )
for file in $interesting_files; do
  generateSymbols $file
  echo "$file"
done
interesting_files2=$(cd ${SOURCE_DIR}_symbols && find . -name '*.sym' )
for file in $interesting_files2; do
  moveSymbols $file
done
