# Specify compiler
CC=gcc

# Specify linker
LINK=gcc

# Compiler and linker flags for production
CFLAGS=-O2 -DNDEBUG
LDFLAGS=

# Source files
SRC=src/arraylist.c src/main.c

# Output binary
OUT=main.out

run: $(OUT)
	@./$(OUT)

$(OUT): $(SRC)
	@$(CC) $(CFLAGS) -o $(OUT) $(SRC) $(LDFLAGS)
