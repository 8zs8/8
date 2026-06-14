# Quick Web Launcher - Makefile for MinGW (MinGW-w64 / TD-GCC)
#
# Usage:
#   mingw32-make              # builds installer.exe
#   mingw32-make app          # builds app.exe only
#   mingw32-make clean        # removes .o/.exe
#
# Notes:
#   - Requires: g++, windres (both in the same MinGW bin directory).
#   - You must have your custom icon named "app.ico" in this directory.
#   - app.exe must exist before the installer is built, because
#     installer.rc embeds app.exe as RCDATA 101.

APPSRCS = app.cpp app.rc
INSTSRCS = installer.cpp installer.rc

LIBS = -luser32 -lgdi32 -lmsimg32 -lshell32 -lshlwapi -lole32 -luuid -lcomctl32 -lcomdlg32
CFLAGS = -O2 -mwindows -static-libgcc -static-libstdc++

.PHONY: all app installer clean

all: installer.exe

app.exe: $(APPSRCS) app.ico
	windres -o app_res.o app.rc
	g++ $(CFLAGS) -o app.exe app.cpp app_res.o $(LIBS)

installer.exe: installer.cpp installer.rc manifest.xml app.exe
	windres -o inst_res.o installer.rc
	g++ $(CFLAGS) -o installer.exe installer.cpp inst_res.o $(LIBS)

clean:
	-del /Q app_res.o inst_res.o 2>nul
	-del /Q app.exe installer.exe 2>nul
