#include "../../includes/Network.hpp"
#include <csignal>
#include <cstring>

using namespace std;

volatile sig_atomic_t	g_StopRequested = 0;

extern "C" void	handle_sigint(int)
{
	g_StopRequested = 1;
}

void	setup_signals(void)
{
	struct sigaction	act_int_term;
	struct sigaction	act_pipe;

	memset(&act_int_term, 0, sizeof(act_int_term));
	memset(&act_pipe, 0, sizeof(act_pipe));

	act_int_term.sa_handler = handle_sigint;
	act_pipe.sa_handler = SIG_IGN;
	sigemptyset(&act_int_term.sa_mask);
	sigemptyset(&act_pipe.sa_mask);
	act_int_term.sa_flags = 0;
	sigaction(SIGINT, &act_int_term, NULL);
	sigaction(SIGTERM, &act_int_term, NULL);
	sigaction(SIGPIPE, &act_pipe, NULL);
}