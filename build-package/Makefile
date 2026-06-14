# ====================================================================
#  Quick Web Launcher - Makefile for MinGW/GCC
#
#  Compiler: C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe
#  WindRes:  C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\windres.exe
#
#  Usage: Double-click "build.bat" (not this file!)
#
# ====================================================================

# Compiler paths
GCC = C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe
WRC = C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\windres.exe

# Compiler flags
CFLAGS = -mwindows -O2 -static-libgcc -static-libstdc++

# Linker flags - ALL libraries MUST be listed here
# Order matters: libraries after object files
APP_LIBS = -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32
INSTALLER_LIBS = -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32

# Targets
all: app.exe installer.exe

app.exe: app.cpp app_res.o
	@echo Linking app.exe...
	"$(GCC)" $(CFLAGS) -o app.exe app.cpp app_res.o $(APP_LIBS)
	@if exist app.exe echo SUCCESS: app.exe created!

app_res.o: app.rc
	@echo Compiling app.rc...
	"$(WRC)" -o app_res.o app.rc

installer.exe: installer.cpp inst_res.o
	@echo Linking installer.exe...
	"$(GCC)" $(CFLAGS) -o installer.exe installer.cpp inst_res.o $(INSTALLER_LIBS)
	@if exist installer.exe echo SUCCESS: installer.exe created!

inst_res.o: installer.rc
	@echo Compiling installer.rc...
	"$(WRC)" -o inst_res.o installer.rc

clean:
	@if exist app_res.o del /q app_res.o
	@if exist inst_res.o del /q inst_res.o
	@if exist app.exe del /q app.exe
	@if exist installer.exe del /q installer.exe
	@echo Clean complete.

.PHONY: all clean
