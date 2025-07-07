CC = gcc
CFLAGS = -Wall -Wextra -Werror -I. -Iminilibx-linux -I/usr/include -O3
LFLAGS	=	-Lminilibx-linux -lmlx -L/usr/lib -Iminilibx-linux -lXext -lX11 -lm -lz
INCLUDES = -Iinclude

SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:src/%.c=obj/%.o)
TARGET = retromfa
OBJDIR = obj

MFAFILES = $(wildcard assets/MFA/*.mfa)
TESTFILES = $(wildcard assets/testerror/*)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(MAKE) -j -C minilibx-linux
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LFLAGS)

$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(TARGET)

re: fclean all

testerror: $(TARGET)
	@echo "Running tests..."
	./$(TARGET) $(TESTFILES)

testsingle: $(TARGET)
	@echo "Running single test..."
	./$(TARGET) assets/testerror/blue.mfa

run: $(TARGET)
	./$(TARGET) $(MFAFILES)


.PHONY: all clean fclean re run testerror testsingle
