gcc -O3 -s -Wall -I%X11ROOT%/XFree86/include -D__ST_MT_ERRNO__ -DFUNCPROTO=15 -DNARROWPROTO mouseclock.c -o mouseclock.exe -Zmtd -L%X11ROOT%/XFree86/lib -lX11 -Zbin-files -Zmtd -Zomf