NAME = codexion

CC = cc
CFLAGS = -Wall -Wextra -Werror -pthread
CPPFLAGS = -Iinclude

SRC_DIR = src
OBJ_DIR = obj

SRCS = main.c parse.c init.c cleanup.c \
       time_utils.c heap.c dongle.c coder.c \
       monitor.c log.c sim_utils.c dongle_utils.c \
	   init_2.c

OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c include/codexion.h
	mkdir -p $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re