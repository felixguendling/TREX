CC=g++ -fopenmp
FLAGS=-std=c++17 -pipe
OPTIMIZATION=-march=native -O3
DEBUG=-rdynamic -Werror -Wpedantic -pedantic-errors -Wall -Wextra -Wparentheses -Wfatal-errors -D_GLIBCXX_DEBUG -g -fno-omit-frame-pointer
RELEASE=-ffast-math -ftree-vectorize -Wfatal-errors -DNDEBUG -fomit-frame-pointer -mtune=native -fno-stack-protector
SPARSEHASH=-I ../ExternalLibs/sparsehash/include

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
	$(CC) $(FLAGS) $(OPTIMIZATION) $(DEBUG) -o MLTB MLTB.cpp

MLTBRelease:
	$(CC) $(FLAGS) $(OPTIMIZATION) $(RELEASE) -o MLTB MLTB.cpp

MLTB_COLLECT:
	$(CC) $(FLAGS) $(OPTIMIZATION) $(RELEASE) -o MLTB MLTB.cpp -fprofile-generate

MLTB_COMPILE_COLLECT:
	$(CC) $(FLAGS) $(OPTIMIZATION) $(RELEASE) -o MLTB MLTB.cpp -fprofile-use -fprofile-correction
