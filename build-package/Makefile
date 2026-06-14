# ====================================================================
#  Quick Web Launcher - Makefile
#
#  Usage (from Dev-C++ terminal or cmd.exe):
#    mingw32-make -f Makefile app.exe         (build main program)
#    mingw32-make -f Makefile installer.exe   (build installer)
#    mingw32-make -f Makefile all             (build everything)
#    mingw32-make -f Makefile clean           (remove output)
# ====================================================================

GCC     = g++.exe
WRC     = windres.exe
CFLAGS  = -mwindows -O2 -static-libgcc -static-libstdc++
LDFLAGS = -mwindows -static-libgcc -static-libstdc++

APP_LIBS   = -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32
INST_LIBS  = -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32

.PHONY: all app installer clean

all: app.exe installer.exe

# --- app.exe ---
app.exe: app.cpp app_res.o
	$(GCC) $(CFLAGS) -o app.exe app.cpp app_res.o $(APP_LIBS)

app_res.o: app.rc
	$(WRC) -o app_res.o app.rc

# --- installer.exe ---
installer.exe: installer.cpp inst_res.o
	$(GCC) $(CFLAGS) -o installer.exe installer.cpp inst_res.o $(INST_LIBS)

inst_res.o: installer.rc
	$(WRC) -o inst_res.o installer.rc

clean:
	-del /q app.exe installer.exe app_res.o inst_res.o 2>nul
