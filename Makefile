# ============================================================
#  Makefile —— 适合 Dev-C++ / Code::Blocks / 任意自带 GCC 的 C++ IDE
#
#  使用方法（任选其一）：
#    ① 命令行：进入项目目录，输入  mingw32-make  （或 make）
#    ② Dev-C++：打开工程文件后点击"运行 → 编译"
#
#  构建顺序（由 Makefile 自动控制）：
#    1) 编译主程序资源 app.rc → app_res.o
#    2) 编译链接 app.exe
#    3) 编译安装程序资源 installer.rc（内部会嵌入 app.exe）→ inst_res.o
#    4) 编译链接 installer.exe
# ============================================================

CXX       = g++
RC        = windres
CXXFLAGS  = -O2 -mwindows -Wl,--subsystem,windows
LDFLAGS   = -static-libgcc -static-libstdc++
LIBS      = -luser32 -lgdi32 -lshell32 -lole32 -luuid -lcomctl32

.PHONY: all clean app installer

all: app.exe installer.exe

# ---------- 主程序 ----------
app_res.o: app.rc app.ico
	$(RC) -o app_res.o app.rc

app.exe: app.cpp app_res.o
	$(CXX) $(CXXFLAGS) -o app.exe app.cpp app_res.o $(LIBS) $(LDFLAGS)

# 快捷：只编译主程序
app: app.exe

# ---------- 安装程序 ----------
# 关键依赖：inst_res.o 必须在 app.exe 之后生成，
# 因为 installer.rc 会把 app.exe 作为 RCDATA 二进制资源嵌入。
inst_res.o: installer.rc app.ico app.exe
	$(RC) -o inst_res.o installer.rc

installer.exe: installer.cpp inst_res.o
	$(CXX) $(CXXFLAGS) -o installer.exe installer.cpp inst_res.o $(LIBS) $(LDFLAGS)

# 快捷：只编译安装程序
installer: installer.exe

# ---------- 清理 ----------
clean:
	-del app_res.o inst_res.o app.exe installer.exe 2>nul
