all: gameoflife.exe

gameoflife.exe: resource.res
	cl gameoflife.c /link user32.lib gdi32.lib resource.res

resource.res:
	rc resource.rc

