import os

print("Content-Type: text/plain")
print("")
for key in sorted(os.environ):
	print(key + "=" + os.environ[key])
