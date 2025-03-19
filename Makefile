# Compiler
CC = gcc

# MongoDB flags and libs (추가)
MONGO_FLAGS = $(shell pkg-config --cflags libmongoc-1.0)
MONGO_LIBS = $(shell pkg-config --libs libmongoc-1.0)

# Compiler flags
CFLAGS = -Wall -g $(MONGO_FLAGS)

# Libraries to link
# LIBS = -lpthread -lmysqlclient 
LIBS = -lpthread  $(MONGO_LIBS)

# Source files
# SRCS = server.c proc_mysql.c message.c load_config.c parson.c
SRCS = server.c proc_mongo.c message.c load_config.c parson.c

# Object files
OBJS = $(SRCS:.c=.o)

# Output executable
TARGET = eutisync

# Default target to build the executable
all: $(TARGET)

# Rule to build the target
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

# Rule to compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


# Clean up generated files
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean

# prod 빌드
prod: prod_server.o utils.o
	$(CC) -o prod_server prod_server.o utils.o -lpthread

prod_server.o: prod_server.c
	$(CC) -c prod_server.c

utils.o: utils.c
	$(CC) -c utils.c