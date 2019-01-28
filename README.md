# SoundCanceller in oposite Speaker
対面型配置スピーカーの実験に使うソースコードを保存していきます
## alsa-library Install
'$ sudo apt-get install libasound2-dev libesd0'
#ALSA設定
'$ cd /etc/modprobe.d'  
'$ cat alsa-base.conf'

#gcc コンパイル
'$ gcc sin480.c inputKey.c -o sin480 -lm -lasound'


#参考URL
<https://fisproject.jp/2014/07/%E3%80%90linux%E3%80%91alsa%E3%81%AE%E4%BD%BF%E3%81%84%E6%96%B9%E3%80%90aplay%E3%80%91/> 
