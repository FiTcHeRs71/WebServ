/* ************************************************************************** */
/*                                                                            */
/*   test_request.cpp — harnais unitaire pour C-02                            */
/*                                                                            */
/*   Valide setUpContentLength() sans dependre de B-03 (Connection) :          */
/*   les requetes sont fabriquees a la main et poussees dans Feed().           */
/*                                                                            */
/*   Compilation :                                                            */
/*     g++ -Wall -Wextra -Werror -std=c++98 -o test_request \                  */
/*         tests/test_request.cpp srcs/http/Request.cpp \                      */
/*         srcs/http/httpUtils.cpp srcs/config/ (tous)                           */
/*                                                                            */
/*   Lancement (depuis la racine du projet, conf/tester.conf doit exister) :   */
/*     ./test_request                                                         */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Config.hpp"
#include "../includes/Request.hpp"
#include <iostream>
#include <string>
#include <exception>

using namespace std;

static int	g_pass = 0;
static int	g_fail = 0;

/**
 * @brief Rend l'enum EParseResult lisible dans la sortie du test.
 */
static const char	*result_name(EParseResult r)
{
	if (r == REQ_COMPLETE)
		return ("REQ_COMPLETE");
	if (r == REQ_INCOMPLETE)
		return ("REQ_INCOMPLETE");
	return ("REQ_ERROR");
}

/**
 * @brief Joue une requete brute et compare au resultat attendu.
 *
 * @param name      libelle affiche dans le rapport
 * @param raw       la requete complete, \r\n compris
 * @param srv       le ServerConfig pose via SetServerConfig() (peut etre NULL)
 * @param expected  EParseResult attendu
 * @param code      _ErrorCode attendu (0 si pas d'erreur)
 */
static void	run_case(const string &name, const string &raw,
					const ServerConfig *srv,
					EParseResult expected, int code)
{
	Request			req;
	EParseResult	got;
	int				got_code;

	req.SetServerConfig(srv);
	got = req.Feed(raw.data(), raw.size());
	got_code = req.getErrorCode();
	if (got == expected && got_code == code)
	{
		g_pass++;
		cout << "  [ OK ] " << name << endl;
		return ;
	}
	g_fail++;
	cout << "  [ KO ] " << name << endl;
	cout << "         attendu : " << result_name(expected)
		<< " / code " << code << endl;
	cout << "         obtenu  : " << result_name(got)
		<< " / code " << got_code << endl;
}

/**
 * @brief Assemble une requete : ligne de requete + headers + ligne vide.
 *        Le corps n'est volontairement jamais envoye : ce harnais ne teste
 *        que la decision prise a la fin des en-tetes.
 */
static string	build(const string &method, const string &path,
					const string &extra_headers)
{
	string	raw;

	raw = method + " " + path + " HTTP/1.1\r\n";
	raw += "Host: localhost:8080\r\n";
	raw += extra_headers;
	raw += "\r\n";
	return (raw);
}

/**
 * @brief Pousse jusqu'a trois fragments dans le meme Request, comme le ferait
 *        une suite de recv(), et verifie le corps final octet par octet.
 *        Un fragment vide est ignore : passer "" pour n'en jouer que deux.
 *
 * @param expected_body le corps attendu, comparaison sur .size() ET contenu
 */
static void	run_body_case(const string &name,
					const string &c1, const string &c2, const string &c3,
					const ServerConfig *srv,
					EParseResult expected, const string &expected_body)
{
	Request			req;
	EParseResult	got;

	req.SetServerConfig(srv);
	got = req.Feed(c1.data(), c1.size());
	if (!c2.empty())
		got = req.Feed(c2.data(), c2.size());
	if (!c3.empty())
		got = req.Feed(c3.data(), c3.size());
	if (got == expected && req.getBody() == expected_body)
	{
		g_pass++;
		cout << "  [ OK ] " << name << endl;
		return ;
	}
	g_fail++;
	cout << "  [ KO ] " << name << endl;
	cout << "         attendu : " << result_name(expected)
		<< " / corps de " << expected_body.size() << " octets" << endl;
	cout << "         obtenu  : " << result_name(got)
		<< " / corps de " << req.getBody().size() << " octets" << endl;
}

int	main(void)
{
	ConfigParser	cfg;

	cout << "=== C-02 : Content-Length + 413 ===" << endl;
	try
	{
		parse("conf/tester.conf", cfg);
	}
	catch (exception &e)
	{
		cerr << "/!\\ conf/tester.conf illisible : " << e.what() << endl;
		return (1);
	}
	if (cfg.getServers().empty())
	{
		cerr << "/!\\ aucun server dans conf/tester.conf" << endl;
		return (1);
	}

	const ServerConfig	*srv = &cfg.getServers()[0];

	/* --- 1. GET sans Content-Length : pas de corps attendu --- */
	run_case("1. GET sans Content-Length",
		build("GET", "/", ""),
		srv, REQ_COMPLETE, 0);

	/* --- 2. POST sans Content-Length : 411 Length Required --- */
	run_case("2. POST sans Content-Length -> 411",
		build("POST", "/post_body", ""),
		srv, REQ_ERROR, 411);

	/* --- 3. Content-Length: 0 : corps vide, rien a attendre --- */
	run_case("3. POST Content-Length: 0",
		build("POST", "/post_body", "Content-Length: 0\r\n"),
		srv, REQ_COMPLETE, 0);

	/* --- 4. Pile a la limite : accepte, attend les 100 octets --- */
	run_case("4. POST Content-Length: 100 (limite exacte)",
		build("POST", "/post_body", "Content-Length: 100\r\n"),
		srv, REQ_INCOMPLETE, 0);

	/* --- 5. Un octet de trop : 413 --- */
	run_case("5. POST Content-Length: 101 -> 413",
		build("POST", "/post_body", "Content-Length: 101\r\n"),
		srv, REQ_ERROR, 413);

	/* --- 6. Annonce enorme : refusee sans transfert --- */
	run_case("6. POST Content-Length: 999999999 -> 413",
		build("POST", "/post_body", "Content-Length: 999999999\r\n"),
		srv, REQ_ERROR, 413);

	/* --- 7. Valeur non numerique : 400 --- */
	run_case("7. Content-Length: abc -> 400",
		build("POST", "/post_body", "Content-Length: abc\r\n"),
		srv, REQ_ERROR, 400);

	/* --- 8. Suffixe K : legal en .conf, illegal en HTTP --- */
	run_case("8. Content-Length: 100K -> 400",
		build("POST", "/post_body", "Content-Length: 100K\r\n"),
		srv, REQ_ERROR, 400);

	/* --- 9. Content-Length + Transfer-Encoding : ambigu (RFC 7230) --- */
	run_case("9. Content-Length + Transfer-Encoding -> 400",
		build("POST", "/post_body",
			"Content-Length: 10\r\nTransfer-Encoding: chunked\r\n"),
		srv, REQ_ERROR, 400);

	/* --- 10. Valeur negative : 400 --- */
	run_case("10. Content-Length: -1 -> 400",
		build("POST", "/post_body", "Content-Length: -1\r\n"),
		srv, REQ_ERROR, 400);

	/* --- 11. Debordement de long : 400 (ERANGE) --- */
	run_case("11. Content-Length: 99999999999999999999 -> 400",
		build("POST", "/post_body",
			"Content-Length: 99999999999999999999\r\n"),
		srv, REQ_ERROR, 400);

	/* --- 12. Hors /post_body : la limite du server (10 Mo) s'applique --- */
	run_case("12. POST / Content-Length: 5000 (limite server)",
		build("POST", "/", "Content-Length: 5000\r\n"),
		srv, REQ_INCOMPLETE, 0);

	/* --- 13. _Srv absent : DEFAULT_BODY_SIZE protege quand meme --- */
	run_case("13. sans ServerConfig, Content-Length: 2000000 -> 413",
		build("POST", "/post_body", "Content-Length: 2000000\r\n"),
		NULL, REQ_ERROR, 413);

	/* --- 14. Contre-test du 13 : sous DEFAULT_BODY_SIZE, on accepte.
	          Verrouille la VALEUR du defaut, pas seulement son existence. --- */
	run_case("14. sans ServerConfig, Content-Length: 500000 (sous 1 Mo)",
		build("POST", "/post_body", "Content-Length: 500000\r\n"),
		NULL, REQ_INCOMPLETE, 0);

	/* ================= Corps : findBody() ================= */

	/* --- 15. Livraison fragmentee : 3 recv() pour un seul corps --- */
	{
		string	head = build("POST", "/post_body", "Content-Length: 11\r\n");

		run_body_case("15. corps livre en 3 Feed()",
			head + "Hel", "lo wo", "rld",
			srv, REQ_COMPLETE, "Hello world");
	}

	/* --- 16. Corps binaire : des '\0' au milieu ne doivent rien tronquer --- */
	{
		string	bin;
		string	head;

		bin = "AB";
		bin += '\0';
		bin += "CD";
		bin += '\0';
		bin += "EF";                      /* 8 octets, 2 nuls internes */
		head = build("POST", "/post_body", "Content-Length: 8\r\n");
		run_body_case("16. corps binaire avec des \\0",
			head + bin, "", "",
			srv, REQ_COMPLETE, bin);
	}

	/* --- 17. Pipelining : la requete suivante doit survivre dans _Raw --- */
	{
		string			head = build("POST", "/post_body", "Content-Length: 5\r\n");
		string			next = build("GET", "/second", "");
		Request			req;
		EParseResult	got;
		string			raw = head + "Hello" + next;

		req.SetServerConfig(srv);
		got = req.Feed(raw.data(), raw.size());
		if (got == REQ_COMPLETE && req.getBody() == "Hello")
		{
			req.reset();                  /* ce que B fera apres la reponse */
			got = req.Feed("", 0);        /* rien de neuf : tout est deja dans _Raw */
			if (got == REQ_COMPLETE && req.getPath() == "/second")
			{
				g_pass++;
				cout << "  [ OK ] 17. pipelining : 2 requetes en un Feed()" << endl;
			}
			else
			{
				g_fail++;
				cout << "  [ KO ] 17. pipelining : la 2e requete est perdue"
					<< " (path obtenu : \"" << req.getPath() << "\")" << endl;
			}
		}
		else
		{
			g_fail++;
			cout << "  [ KO ] 17. pipelining : la 1re requete a echoue" << endl;
		}
	}

	/* --- 18. Surplus : plus d'octets que Content-Length, on n'avale que 5 --- */
	{
		string	head = build("POST", "/post_body", "Content-Length: 5\r\n");

		run_body_case("18. surplus non avale (CL=5, 10 octets envoyes)",
			head + "HelloTROPLONG", "", "",
			srv, REQ_COMPLETE, "Hello");
	}

	cout << "=== " << g_pass << " OK, " << g_fail << " KO ===" << endl;
	return (g_fail == 0 ? 0 : 1);
}
