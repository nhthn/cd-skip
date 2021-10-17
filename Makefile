all:
	g++ -Ithird_party/tclap/include/ -std=c++14 -lsndfile CDSkip.cpp frontend.cpp -o cdskip
