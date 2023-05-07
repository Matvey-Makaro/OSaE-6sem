echo "one two three four ten" > .words.txt
output=$(echo "One car. Two apples. Three dogs. Four and five. Ten" | ./censor .words.txt)

if [[ $output != "[*] car. [*] apples. [*] dogs. [*] and five. [*]" ]]; then
  echo "ERROR: INCORRECT RESULT: $output"
  return 1
fi

rm -rf .words.txt

echo "TEST PASSED"
