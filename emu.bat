echo off
git init
git add .
set COMMIT_MSG=
set /P COMMIT_MSG=""

git commit -m "%COMMIT_MSG%"
git push emu master