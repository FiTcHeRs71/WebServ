#!/usr/bin/python3
import os, uuid

raw = os.environ.get("HTTP_COOKIE", "")

# parser cote client : meme decoupage que ton getCookies() C++
cookies = {}
for part in raw.split(";"):
    part = part.strip()
    if "=" in part:
        name, _, value = part.partition("=")     # partition = coupe au PREMIER '='
        cookies.setdefault(name.strip(), value.strip())   # setdefault = le premier gagne

known = "sessionid" in cookies

print("Content-Type: text/html")
if not known:
    sid = uuid.uuid4().hex
    print("Set-Cookie: sessionid=" + sid + "; Path=/; HttpOnly")
    print("Set-Cookie: visits=1; Path=/; Max-Age=3600")
else:
    brut = cookies.get("visits", "0")
    n = int(brut) + 1 if brut.isdigit() else 1
    print("Set-Cookie: visits=" + str(n) + "; Path=/; Max-Age=3600")
print()                          # <-- ligne vide : fin des headers. Sans elle -> 502
print("<h1>" + ("Re-bonjour" if known else "Bienvenue") + "</h1>")
print("<p>HTTP_COOKIE recu : " + (raw or "(rien)") + "</p>")
