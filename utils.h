#pragma once

#include <Print.h>

#ifndef HWSERIAL
#define HWSERIAL Serial
#endif

#define SerialPrintf(fmt, ...) SerialPrintfOut(HWSERIAL, 0, fmt, ##__VA_ARGS__)
#define SerialPrintfln(fmt, ...) SerialPrintfOut(HWSERIAL, 1, fmt, ##__VA_ARGS__)
void SerialPrintfOut(Print &output, bool newline, const char* fmt, ...);
