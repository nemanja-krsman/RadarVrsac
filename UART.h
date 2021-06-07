#define SBIT_TXEN     5
#define SBIT_SPEN     7
#define SBIT_CREN     4
#define _XTAL_FREQ 4000000

void UART_Init(int baudRate);
void UART_TxChar(char ch);
char UART_RxChar();
void UART_putst(register const char *str);