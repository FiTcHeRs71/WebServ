# Program name
NAME = webserv

# Directories
SRC_DIR = srcs
INC_DIR = includes
OBJ_DIR = objs

# >>> SRCS AUTO-GENERATED >>>
SRCS_MAIN = $(addprefix $(SRC_DIR)/, \
	main.cpp)

SRCS_CGI = $(addprefix $(SRC_DIR)/cgi/, \
	cgi.cpp \
	CgiProcess.cpp)

SRCS_CONFIG = $(addprefix $(SRC_DIR)/config/, \
	config.cpp \
	listen.cpp \
	LocationConfig.cpp \
	ServerConfig.cpp \
	utils.cpp)

SRCS_HTTP = $(addprefix $(SRC_DIR)/http/, \
	http.cpp \
	Request.cpp \
	Response.cpp)

SRCS_NETWORK = $(addprefix $(SRC_DIR)/network/, \
	Connection.cpp \
	EventLoop.cpp \
	ListenSockets.cpp \
	network.cpp)

# Source files
SRCS = $(SRCS_MAIN) $(SRCS_CGI) $(SRCS_CONFIG) $(SRCS_HTTP) $(SRCS_NETWORK)
# <<< SRCS AUTO-GENERATED <<<

# Object files
OBJS = $(SRCS:%.cpp=$(OBJ_DIR)/%.o)
DEPS = $(OBJS:.o=.d)

# Compiler and flags
# On prefere clang++ (compilateur des macs de l'ecole) pour avoir localement
# les memes diagnostics -Werror qu'a la soutenance. Repli sur c++ si clang++
# n'est pas installe. Override possible : make CXX=g++
CXX := $(shell command -v clang++ >/dev/null 2>&1 && echo clang++ || echo c++)
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I$(INC_DIR) -g
DEPFLAGS = -MMD -MP

# -fstandalone-debug n'existe que chez Clang (macs de l'ecole) : il force
# l'emission des infos de debug completes, que Clang tronque par defaut.
# GCC (Linux) les emet deja avec -g et rejette le flag, ce qui cassait
# la compilation. On ne l'ajoute donc que si le compilateur est Clang.
IS_CLANG := $(shell $(CXX) --version 2>/dev/null | grep -ci clang)
ifneq ($(IS_CLANG),0)
    CXXFLAGS += -fstandalone-debug
endif

# Colors
GREEN = \033[0;32m
CYAN = \033[0;36m
YELLOW = \033[0;33m
RED = \033[0;31m
RESET = \033[0m

# Rules
all: $(NAME)
	@echo "$(GREEN)🎉 $(NAME) ready! 🎉$(RESET)"

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

.PHONY: _compile
_compile:
	$(call spin,🛠  Compiling sources...,$(MAKE) --no-print-directory $(OBJS))

$(NAME): $(OBJS) | _compile
	$(call spin,🔗 Linking $(NAME)...,$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME))
	@echo "$(GREEN)✓ $(NAME) created successfully!$(RESET)"

-include $(DEPS)

clean:
	@rm -rf $(OBJ_DIR)
	@echo "$(CYAN)✓ Object files removed$(RESET)"

fclean: clean
	@rm -f $(NAME)
	@echo "$(CYAN)✓ $(NAME) removed$(RESET)"

re: fclean all

run: all
	@echo "$(GREEN)🚀 Running $(NAME)...$(RESET)"
	@./$(NAME)

.PHONY: all clean fclean re run

define spin
	@printf "$(CYAN)$(1)$(RESET)  "; \
	log=$$(mktemp); \
	( $(2) ) > $$log 2>&1 & pid=$$!; \
	while kill -0 $$pid 2>/dev/null; do \
		for f in ⠋ ⠙ ⠹ ⠸ ⠼ ⠴ ⠦ ⠧ ⠇ ⠏; do \
			printf "\b$$f"; sleep 0.08; \
			kill -0 $$pid 2>/dev/null || break; \
		done; \
	done; \
	wait $$pid; rc=$$?; \
	if [ $$rc -eq 0 ]; then \
		printf "\b$(GREEN)✓$(RESET)\n"; rm -f $$log; \
	else \
		printf "\b$(RED)✗$(RESET)\n"; cat $$log; rm -f $$log; exit $$rc; \
	fi
endef
