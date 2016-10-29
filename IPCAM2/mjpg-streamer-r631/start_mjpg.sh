

export LD_LIBRARY_PATH=/lib/mjpg
[ -d pics ] || { mkdir pics; };

#mjpg_streamer -i "input_uvc.so -d /dev/video2"  -o "output_file.so -f pics -d 500 -l /bin/lcd /tmp/snapshot.jpg";


#/bin/mjpg_streamer -i "input_uvc.so --device=/dev/video0"  -o "output_file.so -c /bin/lcd" -o "output_http.so -w /myhome" &
./mjpg_streamer -i "./input_uvc.so --device=/dev/video0"  -o "./output_http.so -w ./www" 
