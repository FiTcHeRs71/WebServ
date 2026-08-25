#ifndef NETWORK_HPP
# define NETWORK_HPP

# include <iostream>
# include <csignal>

extern	volatile sig_atomic_t g_StopRequested;

void	setup_signals(void);

#endif
