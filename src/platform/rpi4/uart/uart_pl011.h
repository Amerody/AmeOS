#pragma once

void uart_pl011_enable(unsigned int id);

void uart_pl011_putc(unsigned int port, char c);

bool uart_pl011_getc(unsigned int port, char* c);