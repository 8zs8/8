# Quick Web Launcher - Makefile
# Usage: mingw32-make -f Makefile
#   (Run this from the folder that contains all the source files and app.ico)

CC      = g++
WINDRES = windres
CFLAGS  = -mwindows -O2 -static-libgcc -static-libstdc++
RCFLAGS =

# Libraries for app.exe
APP_LIBS = -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32

# Libraries for installer.exe
INST_LIBS = -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32

# --- Targets ---------------------------------------------------------

app: app_res.o
	$(CC) $(CFLAGS) -o app.exe app.cpp app_res.o $(APP_LIBS)

app_res.o: app.rc
	$(WINDRES) -o app_res.o app.rc

installer: inst_res.o
	$(CC) $(CFLAGS) -o installer.exe installer.cpp inst_res.o $(INST_LIBS)

inst_res.o: installer.rc
	$(WINDRES) -o inst_res.o installer.rc

.PHONY: app installer clean

clean:
	del /q app.exe app_res.o installer.exe inst_res.o 2>nul
