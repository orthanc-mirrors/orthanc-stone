# echo "+----------------------+"
# echo "|    template.in.ts    |"
# echo "+----------------------+"

# tsc -t ES2015 .\template.in.ts; node .\template.in.js

echo "+----------------------+"
echo "|    playground.ts     |"
echo "+----------------------+"

tsc -t ES2015 .\playground.ts; node .\playground.js

echo "+----------------------+"
echo "|    playground3.ts     |"
echo "+----------------------+"

tsc -t ES2015 .\playground3.ts; node .\playground3.js


