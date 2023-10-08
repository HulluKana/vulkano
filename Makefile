include .env

CODE = main.cpp vulkano/Backend/src/*.cpp $(IMGUI)/*.cpp
HEADERS = vulkano/*.hpp vulkano/Backend/Headers/*.hpp

vertSources = $(shell find ./vulkano/Backend/Shaders -type f -name "*.vert")
fragSources = $(shell find ./vulkano/Backend/Shaders -type f -name "*.frag")
vertObjFiles = $(patsubst %.vert, %.vert.spv, $(vertSources))
fragObjFiles = $(patsubst %.frag, %.frag.spv, $(fragSources))

COMPILER_FLAGS = -O2 -Wall
LINKER_FLAGS = -I$(TINY_OBJ_LOADER) -I$(IMGUI) -I$(STB_IMAGE) -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

.PHONY: run clean shaders

run: main shaders
	./main

debug: mainWithDebug shaders
	gdb ./mainWithDebug

clean:
	rm -f main
	rm -f mainWithDebug
	rm -f $(vertObjFiles) $(fragObjFiles)

main: $(CODE) $(HEADERS)
	g++ $(COMPILER_FLAGS) -o main $(CODE) $(LINKER_FLAGS)
	
mainWithDebug: $(CODE) $(HEADERS)
	g++ $(COMPILER_FLAGS) -g -o mainWithDebug $(CODE) $(LINKER_FLAGS)

shaders: $(vertObjFiles) $(fragObjFiles)

%.spv: %
	$(GLSLC) $< -o $@