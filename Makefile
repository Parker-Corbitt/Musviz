#Makefile for example sfml project

CXX=clang++ -std=c++17
CXX_FLAGS=-Wall -O3#-Werror
SOURCE=musviz.cc
INCLUDE=-I /opt/homebrew/Cellar/sfml/3.0.1/include
LIBS=-L /opt/homebrew/Cellar/sfml/3.0.1/lib
LINKS=-lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -lsfml-network

all: musviz.cc
	$(CXX) $(CXX_FLAGS) $(SOURCE) $(INCLUDE) $(LIBS) $(LINKS) -o musviz

debug: musviz.cc
	$(CXX) $(CXX_FLAGS) -g $(SOURCE) $(INCLUDE) $(LIBS) $(LINKS) -o musviz

clean: 
	rm musviz musviz/ build/ SFML*
