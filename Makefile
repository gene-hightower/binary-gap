CXXFLAGS := -flto -std=c++2a -Wall -Wextra -O3

all:: binary-gap

clean::
	rm -f binary-gap
