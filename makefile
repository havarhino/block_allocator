CC = gcc
AR = ar
CFLAGS = -Wall -Wextra -g
ARFLAGS = rcs
TEST_CFLAGS = -Wall -Wextra -g -fprofile-arcs -ftest-coverage -DENABLE_DEBUG_HEADER=1 -DTEST_MALLOC -DENABLE_STOMP_DETECT -DTEST_ASSERT
LDFLAGS = -fprofile-arcs -ftest-coverage
SRCS = block_allocator.c
PROXY_SRC = proxy_malloc.c proxy_assert.c
TEST_SRC = test_block_allocator.c
SAMPLE = sample
SAMPLE_SRC = sample_client.c
OBJS = $(SRCS:.c=.o)
PROXY_OBJ = $(PROXY_SRC:.c=.o)
TEST_OBJ = $(TEST_SRC:.c=.o)
SAMPLE_OBJ = $(SAMPLE_SRC:.c=.o)
LIB = libblockallocator.a
TEST_LIB = libblockallocator_test.a
TEST_TARGET = test_block_allocator
INCLUDE_DIR = include
LIB_DIR = lib

.PHONY: all test coverage clean install

all: $(LIB) $(SAMPLE)

$(SAMPLE): install $(SAMPLE_OBJ)
	$(CC) $(LDFLAGS) -o $@ $(SAMPLE_OBJ) -L$(LIB_DIR) -lblockallocator

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJS)

$(TEST_LIB): $(SRCS)
	$(CC) $(TEST_CFLAGS) -c $(SRCS) -o $(SRCS:.c=.test.o)
	$(AR) $(ARFLAGS) $(TEST_LIB) $(SRCS:.c=.test.o)

$(TEST_TARGET): $(TEST_LIB) $(PROXY_OBJ) $(TEST_OBJ)
	$(CC) $(LDFLAGS) -o $@ $(TEST_OBJ) $(PROXY_OBJ) $(TEST_LIB)

%.o: %.c
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

%.test.o: %.c
	$(CC) $(TEST_CFLAGS) -c $< -o $@

$(PROXY_OBJ) $(TEST_OBJ): %.o: %.c
	$(CC) $(TEST_CFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

coverage: clean $(TEST_TARGET)
	./$(TEST_TARGET)
	gcov -o $(SRCS:.c=.test.o) $(SRCS)
	@echo "Coverage report generated. Check *.gcov files."

install: $(LIB)
	mkdir -p $(LIB_DIR) $(INCLUDE_DIR)
	cp $(LIB) $(LIB_DIR)/
	cp block_allocator.h $(INCLUDE_DIR)/
	@echo "Library and header installed to $(LIB_DIR)/ and $(INCLUDE_DIR)/"

clean:
	rm -f $(OBJS) $(PROXY_OBJ) $(TEST_OBJ) $(LIB) $(TEST_LIB) $(SAMPLE_OBJ) $(SAMPLE) *.test.o *.gcno *.gcda *.gcov
	rm -f test_block_allocator

cleaner:
	rm -f $(OBJS) $(PROXY_OBJ) $(TEST_OBJ) $(LIB) $(TEST_LIB) $(SAMPLE_OBJ) $(SAMPLE) *.test.o *.gcno *.gcda *.gcov
	rm -rf $(LIB_DIR) $(INCLUDE_DIR)
	rm -f test_block_allocator
