echo Current dir is "%~dp0"
echo Check if we can find the executable on %~dp0\bin\drgcs.exe
if exist %~dp0\bin\drgcs.exe (
echo TRUE
) else (
echo FALSE
echo Can't find executable exiting
exit
)
cscript %~dp0\bin\makeShortcut.vbs /target:"%~dp0\bin\drgcs.exe" /filename:"%~dp0\dRonin GCS"
