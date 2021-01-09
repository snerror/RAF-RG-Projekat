.PHONY : build clean runonly run

all: | build

clean:
	@rm -rf build

build:
	@mkdir -p build
	@cd build && cmake .. && make

runonly:
	@build/bin/RAF_RG_Projekat_VladetaPutnikovic

run: | all runonly
