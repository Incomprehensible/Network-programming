TARGET=crypto
TARGET1=client
SRC=src/main.c src/serverlib.c src/netlib.c
SRC1=src/client.c
INC=/includes
LIB=-lcrypto -lssl
FLAGS-iCore=-march=corei7-avx -flto -ffunction-sections -fdata-sections -W --data-sections -Os -O2 \
			-ftree-vectorize -funsafe-math-optimizations -pipe
FLAGS-ARM= -march=armv8-a+crc -mtune=cortex-a53 -mfpu=crypto-neon-fp-armv8 -flto -mfloat-abi=hard \
			-ftree-vectorize -funsafe-math-optimizations -Os -O2 -pipe

all: clean $(TARGET) $(TARGET1)

$(TARGET):
	gcc -I/usr/include/openssl -I$(INC) $(SRC) $(LIB) -o $(TARGET) $(FLAGS-iCore)
	gcc -I/usr/include/openssl -I$(INC) $(SRC1) $(LIB) -o $(TARGET1) $(FLAGS-iCore)


clean:
	-rm $(TARGET)
	-rm $(TARGET1)
