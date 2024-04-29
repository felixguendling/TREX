CC=g++ -fopenmp
FLAGS=-std=c++17 -pipe
OPTIMIZATION=-march=native -O3
DEBUG=-rdynamic -Werror -Wpedantic -pedantic-errors -Wall -Wextra -Wparentheses -Wfatal-errors -D_GLIBCXX_DEBUG -g -fno-omit-frame-pointer
RELEASE=-ffast-math -ftree-vectorize -Wfatal-errors -DNDEBUG -fomit-frame-pointer -mtune=native -fno-stack-protector -mavx2 -mno-avx256-split-unaligned-load -mno-avx256-split-unaligned-store -funroll-loops
KAHIP=-I../../ExternalLibs/KaHIP/deploy/ -L../../ExternalLibs/KaHIP/deploy/ -lkahip

all: NestedBipartitionRelease

NestedBipartitionDebug:
	$(CC) $(FLAGS) $(OPTIMIZATION) $(DEBUG) -o NestedBipartition RunNestedBiPartition.cpp $(KAHIP)

NestedBipartitionRelease:
	$(CC) $(FLAGS) $(OPTIMIZATION) $(RELEASE) -o NestedBipartition RunNestedBiPartition.cpp $(KAHIP)
