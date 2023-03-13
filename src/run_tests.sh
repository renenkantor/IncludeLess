#! /bin/bash
parent_path=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
cd "$parent_path"

cd backend

pwd_v=$(pwd)

pwd_test="$pwd_v/../tests/"
output_path="$pwd_v/../run_test_output.txt"

make &> "$output_path"
echo "Finished build"
echo $pwd_test >> "$output_path" 2>&1
for file in $pwd_test*
do
  echo "Running $file/main.cpp" >> "$output_path" 2>&1
  ./main.exe "$file/main.cpp" &> /dev/null
  if [[ -e "$file/useless_includes_ref.txt" && -e "$file/useless_includes.txt" ]]; then
    if ! diff "$file/useless_includes_ref.txt" "$file/useless_includes.txt" >> "$output_path" 2>&1; then
      echo "Diff failed for file $file/useless_includes.txt"
    fi
  fi
  if [[ -e "$file/all_includes_ref.txt" && -e "$file/all_includes.txt" ]]; then
    if ! diff "$file/all_includes_ref.txt" "$file/all_includes.txt" >> "$output_path" 2>&1; then
      echo "Diff failed for file $file/all_includes.txt"
    fi
  fi
done
echo "Finished tests"