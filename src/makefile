# Directories:
LIBS = `allegro-config --libs`
INCLUDES = `allegro-config --cflags`
# gcc partest.c -o partest -O2 `allegro-config --libs`

# Compiler:
CC = gcc

# Compiler options: Compile only, speed optimizations, all warnings, warnings are errors
CCOPTS = -c -O2 -Wall $(INCLUDES) # -Werror

# Linker: gcc
LINK = gcc

# Targets:

gblnk :       gblnk.c
		$(CC) gblnk.c -o gblnk -O2 -Wall $(LIBS)
