VULKAN_SDK      =   "%VK_SDK_PATH%"

CC			    =   clang++
CXXFLAGS        =   -std=c++23 -fexec-charset=UTF-8 -m64 -w
INCLUDE         =   -I $(VULKAN_SDK)/Include -I ../../External/ -I ../../External/boost_1_88_0

build:
	$(CC) $(CXXFLAGS) -O3 $(INCLUDE) ./VGGenerator.cpp \
	../../External/meshoptimizer/src/*.cpp \
	../../Source/VirtualGeometry/VG_File.cpp \
	-O3 -o ./vg_generator.exe
#-g3 -O0 -fno-limit-debug-info -o ./vg_generator.exe