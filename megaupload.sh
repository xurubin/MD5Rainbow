#!/bin/bash

# place your megaupload cookie string here
# to obtain one, all you need to do is login to megaupload
# type the following in address bar
# javascript:document.write(document.cookie);
# your string is user=(something);
cstr=c7c55b0c58970856fe2dfad37a2c9a4a
description=test

for file in "$@"
do
 echo "Computing MD5...$file..."
 description=`md5sum -b $file`
 echo -n "Uploading...$file..."
 ident=`wget -qO- --no-cookies --header "Cookie: user=$cstr" http://www.megaupload.com/ | grep ENCTYPE | cut -d '"' -f 8`
 value=`echo $ident | cut -d '=' -f 2`
 data=`curl -s -b "user=$cstr" -F "UPLOAD_IDENTIFIER=$ident" -F "sessionid=$value" -F "file=@$file" -F "message=$description" -F "accept=on" $ident`
 size=`echo $data | cut -d ' ' -f 10``echo $data | cut -d ' ' -f 11`
 mega=`echo $data | cut -d "'" -f 6 | cut -d '=' -f 2`
 echo "$file||$size||$mega" >> links
 echo "Done!"
done

