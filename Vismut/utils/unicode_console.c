#include "unicode_console.h"

#include <fcntl.h>
#include <io.h>
#include <stdio.h>

void EnableUnicodeConsole() {
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);
}
