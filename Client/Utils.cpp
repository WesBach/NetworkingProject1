#include <stdio.h>
#include <Windows.h>

HANDLE hStdout;
CONSOLE_CURSOR_INFO lpCursor;
COORD coord = { 0,0 };
int max_number_of_rows = 10;
char buffer[1024];

void start_text() {
	GetConsoleCursorInfo(hStdout, &lpCursor);
	lpCursor.bVisible = false;
	SetConsoleCursorInfo(hStdout, &lpCursor);

	coord.Y = 10;
}

void end_text() {
	coord.Y = 10;
}

//Improve this funciton as needed....
void print_text(const char* text, ...) {

	va_list argptr;
	va_start(argptr, text);
	vsprintf(buffer, text, argptr);
	va_end(argptr);

	int len = strlen(buffer);

	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	SetConsoleCursorPosition(hStdout, coord);
	coord.Y++;
	WriteConsole(hStdout, "", 1, NULL, NULL);
	WriteConsole(hStdout, buffer, len, NULL, NULL);

	//memset(&buffer[0], 0, sizeof(buffer));
}
