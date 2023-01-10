mkdir build-web

emcc -c -o build-web\miniaudio.o  ray\miniaudio.c  -DPLATFORM_WEB

emcc -c -o build-web\raygui.o ray\raygui.c   -DPLATFORM_WEB 

emcc -o build-web\game.html ray\main.cpp build-web\raygui.o build-web\miniaudio.o ..\raylib_em_install\libraylib.a -L..\raylib_em_install\libraylib.a -s USE_GLFW=3 --shell-file ray\shell.html -DPLATFORM_WEB -s ASYNCIFY