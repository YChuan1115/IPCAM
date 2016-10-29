#!/usr/bin/env python

from subprocess import check_output
from os import system
from os import getcwd

pre_ip = ''
now_ip = ''

def make_view_html_with_ip(from_template ,to_html_file, _ip):
    _msg = ['sed','s/#SERV_IP#/%s/g' % (_ip), from_template , '>' , to_html_file]
    system(' '.join(_msg))

if __name__ == '__main__':
    now_path = getcwd()
    system('%s/readkey/ipcam &' % now_path)
    system('%s/mjpg-streamer-r631/mjpg_streamer -i "%s/mjpg-streamer-r631/input_uvc.so --device=/dev/video0"  -o "%s/mjpg-streamer-r631/output_http.so -w %s/mjpg-streamer-r631/www"' % (now_path,now_path, now_path, now_path))
    while(1):
        now_ip = check_output(['ifconfig','eth0']).split('\n')[1].strip().split()[1].split(':')[1]
        if (now_ip != pre_ip):
            make_view_html_with_ip('./mjpg-streamer-r631/www/view_template.html','./mjpg-streamer-r631/www/view.html', now_ip)
            make_view_html_with_ip('./mjpg-streamer-r631/www/index_template.html','./mjpg-streamer-r631/www/index.html',now_ip)
            pre_ip = now_ip
