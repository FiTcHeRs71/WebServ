===Utilisation et test des ListenSockets===

TEST1
{
	-Ouvrir 2 terminals
	-Lancer nc -l (port) -> terminal 1
	-Lancer ./WebServ *.conf (avec le meme port) -> terminal 2
	-Doit echouer
}

TEST2
{
	-Ouvrir 2 terminals
	-Mettre un sleep en fin de main
	-lancer ./WebServ *.conf -> terminal 1
	-Lancer la commande ss -lnt -> terminal 2
	-Doit afficher le port ouvert en listen only
}
