cd ..
zipName="../Backup/gxcc/gxcc.$(date +"%Y.%m.%d").7z"
echo $zipName

files="SciTEDirectory.properties cc/*.properties gx/*.properties"
#~ compiler files
files="$files cc/src/*.* cc/*.sh cc/*.cmd cc/*.cvx gx/tests/*.*"
#~ graphics files
files="$files gx/src/*.* gx/*.sh gx/*.cmd gx/*.gxc gx/tests/*.*"
#~ media files
files="$files gx/*.ini gx/media/scripts/*.* gx/media/fonts/*.*"

for file in $files
do
	#~ echo "**** downloading: $file"
	7z a $zipName $file
done

#~ FOR %%f IN (%files%) DO IF EXIST %%f (
#~ %zipapp% %zipName% %%f
#~ IF ERRORLEVEL 1 GOTO QUIT
#~ REM ~ echo %%f
#~ )
