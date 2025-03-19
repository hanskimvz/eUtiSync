CC = gcc
CFLAGS = -Wall -Wextra -g -D_GNU_SOURCE -I..
LDFLAGS = -lpthread

# 소스 파일 및 객체 파일 정의
COMMON_SRC = ../message.c ../load_config.c ../parson.c
COMMON_OBJ = $(COMMON_SRC:.c=.o)

SERIALIZER_SRC = serializer.c
SERIALIZER_OBJ = $(SERIALIZER_SRC:.c=.o)
SERIALIZER_TARGET = serializer

.PHONY: all clean

all: serializer

# serializer 빌드 규칙
serializer: $(SERIALIZER_OBJ) $(COMMON_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

# C 소스 파일 컴파일 규칙
%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

# 상위 디렉토리의 소스 파일 컴파일 규칙
../%.o: ../%.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(SERIALIZER_OBJ) $(SERIALIZER_TARGET) $(COMMON_OBJ)

# 의존성 정의
serializer.o: serializer.c ../message.h ../load_config.h
../message.o: ../message.c ../message.h ../load_config.h
../load_config.o: ../load_config.c ../load_config.h ../parson.h
../parson.o: ../parson.c ../parson.h 