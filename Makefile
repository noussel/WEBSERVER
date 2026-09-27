NAME = webserv

CXX = g++

CXXFLAGS = -Wall -Wextra -Werror -std=c++98  -fsanitize=address -Iparsing

SRC = main.cpp parsing/ConfigParser.cpp parsing/request-parse.cpp \
	src/Client.cpp src/Server.cpp Response/Get.cpp  Response/Delete.cpp  Response/Post.cpp Response/Response.cpp

OBJ = $(SRC:.cpp=.o)

TEST_NAME = tests/parser_tests
TEST_SRC = tests/parser_tests.cpp parsing/request-parse.cpp

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm $(NAME)

re: clean all

.PHONY: all clean fclean re

test: $(TEST_NAME)

$(TEST_NAME): $(TEST_SRC)
	mkdir -p tests
	$(CXX) $(CXXFLAGS) -Iincludes -o $(TEST_NAME) $(TEST_SRC)

.PHONY: test
