CC=g++ -fopenmp
FLAGS=-std=c++17 -pipe
OPTIMIZATION=-march=native -O3
DEBUG=-rdynamic -Werror -Wpedantic -pedantic-errors -Wall -Wextra -Wparentheses -Wfatal-errors -D_GLIBCXX_DEBUG -g -fno-omit-frame-pointer
RELEASE=-ffast-math -ftree-vectorize -Wfatal-errors -DNDEBUG -fomit-frame-pointer -mtune=native -fno-stack-protector -mavx2 -mno-avx256-split-unaligned-load -mno-avx256-split-unaligned-store -funroll-loops
SPARSEHASH=-I ../ExternalLibs/sparsehash/include
TBB=-ltbb

all: MLTBRelease
# all: TPDebug ArcTBDebug NetworkDebug ULTRADebug

NetworkDebug:
	$(CC) $(FLAGS) $(OPTIMIZATION) $(DEBUG) -o Network Network.cpp

NetworkRelease:
	$(CC) $(FLAGS) $(OPTIMIZATION) $(RELEASE) -o Network Network.cpp

ULTRADebug:
	$(CC) $(FLAGS) $(OPTIMIZATION) $(DEBUG) -o ULTRA ULTRA.cpp

ULTRARelease:
	$(CC) $(FLAGS) $(OPTIMIZATION) $(RELEASE) -o ULTRA ULTRA.cpp

ArcTBDebug:
	$(CC) $(FLAGS) $(OPTIMIZATION) $(DEBUG) -o ARCTB ArcTB.cpp

ArcTBRelease:
	$(CC) $(FLAGS) $(OPTIMIZATION) $(RELEASE) -o ARCTB ArcTB.cpp

TPDebug:
	$(CC) $(FLAGS) $(OPTIMIZATION) $(DEBUG) $(SPARSEHASH) -o TP TP.cpp

TPRelease:
	$(CC) $(FLAGS) $(OPTIMIZATION) $(RELEASE) $(SPARSEHASH) -o TP TP.cpp

MLTBDebug:
	$(CC) $(FLAGS) $(OPTIMIZATION) $(DEBUG) -o MLTB MLTB.cpp $(TBB)

MLTBRelease:
	$(CC) $(FLAGS) $(OPTIMIZATION) $(RELEASE) -o MLTB MLTB.cpp $(TBB)
