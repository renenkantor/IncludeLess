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

function recursive_files {
  for file in "$1"/*; do
    if [ -d "$file" ]; then
      recursive_files "$file" "$2"
    else
      extension="${file##*.}"
      if [[ "$extension" == "cpp" || "$extension" == "c" || "$extension" == "h" || "$extension" == "cxx" ]]; then
        if [[ -e "$2/.tmp_extension/$(basename "$file")_useless_includes.txt" && -e "$2/reference_result/$(basename "$file")_useless_includes.txt" ]]; then
          if ! diff "$2/.tmp_extension/$(basename "$file")_useless_includes.txt" "$2/reference_result/$(basename "$file")_useless_includes.txt" >> "$output_path" 2>&1; then
            echo "Diff failed for file $2/.tmp_extension/$(basename "$file")_useless_includes.txt"
          fi
        fi
        if [[ -e "$2/.tmp_extension/$(basename "$file")_all_includes.txt" && -e "$2/reference_result/$(basename "$file")_all_includes.txt" ]]; then
          if ! diff "$2/.tmp_extension/$(basename "$file")_all_includes.txt" "$2/reference_result/$(basename "$file")_all_includes.txt" >> "$output_path" 2>&1; then
            echo "Diff failed for file $2/.tmp_extension/$(basename "$file")_all_includes.txt"
          fi
        fi
        if [[ -e "$2/.tmp_extension/$(basename "$file")_ranges.txt" && -e "$2/reference_result/$(basename "$file")_ranges.txt" ]]; then
          if ! diff "$2/.tmp_extension/$(basename "$file")_ranges.txt" "$2/reference_result/$(basename "$file")_ranges.txt" >> "$output_path" 2>&1; then
            echo "Diff failed for file $2/.tmp_extension/$(basename "$file")_ranges.txt"
          fi
        fi
      fi
    fi
  done
}

for file in $pwd_test*
do
  echo "Running $file" >> "$output_path" 2>&1
  ./main.exe "$file" &> /dev/null
  recursive_files "$file" "$file"
done
echo "Finished tests"