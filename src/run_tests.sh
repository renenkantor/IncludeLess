#! /bin/bash
parent_path=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
cd "$parent_path"

cd backend
make
echo "Finished build"
pwd_v=$(pwd)

pwd_test="$pwd_v/../tests/"
for file in $pwd_test*
do
  echo "Running $file/main.cpp"
  ./main.exe "$file/main.cpp" &> /dev/null
  diff "$file/useless_includes_ref.txt" "$file/useless_includes.txt"
  diff "$file/all_includes_ref.txt" "$file/all_includes.txt"
done
