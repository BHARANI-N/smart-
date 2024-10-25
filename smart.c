#include <lpc214x.h>   // Include LPC2148 register definitions
#include <string.h>    // For string handling functions like strcpy, strcmp
#include <stdio.h>     // For sprintf function

// Function Prototypes
void UART0_Init(void);
void UART0_SendChar(char data);
char UART0_ReceiveChar(void);
void UART0_SendString(const char *str);
void delay(unsigned int count);
void LCD_Init(void);
void LCD_Command(unsigned char command);
void LCD_Data(unsigned char data);
void LCD_DisplayString(const char *str);
void LCD_Clear(void);

// Simulating a simple database for products and prices using RFID tags
struct Product {
    char rfid[12];   // RFID tag (unique identifier)
    char name[20];   // Product name
    float price;     // Product price
};

// Simulated products and prices
struct Product products[] = {
    {"123456789ABC", "Milk", 1.50},
    {"23456789ABCD", "Bread", 0.75},
    {"3456789ABCDE", "Eggs", 2.00},
    {"456789ABCDEF", "Butter", 1.20}
};

#define TOTAL_PRODUCTS 4
#define RFID_TAG_LENGTH 12   // Max length of RFID tag (12 characters)
#define MAX_LCD_LINES 16     // Max characters per line on LCD
float total_price = 0;

// UART0 initialization function for UART0 (9600 baud rate)
void UART0_Init(void) {
    PINSEL0 |= 0x00000005;   // Enable TxD0 and RxD0 for UART0
    U0LCR = 0x83;            // 8 bits, no Parity, 1 Stop bit, DLAB = 1
    U0DLM = 0x00;            // Set baud rate to 9600 (DLL and DLM)
    U0DLL = 0x61;            // 9600 Baud Rate @ 15MHz PCLK
    U0LCR = 0x03;            // DLAB = 0
}

// Function to send a character via UART0
void UART0_SendChar(char data) {
    while (!(U0LSR & 0x20));  // Wait until the THR is empty
    U0THR = data;
}

// Function to receive a character via UART0
char UART0_ReceiveChar(void) {
    while (!(U0LSR & 0x01));  // Wait until data is received
    return U0RBR;
}

// Function to send a string via UART0
void UART0_SendString(const char *str) {
    while (*str) {
        UART0_SendChar(*str++);
    }
}

// Function to match RFID with a product and add the price
void check_and_add_product(char *rfid_tag) {
	      char price_str[10];
	      int i;
    for ( i = 0; i < TOTAL_PRODUCTS; i++) {
        if (strcmp(products[i].rfid, rfid_tag) == 0) {
            LCD_Clear();
            LCD_DisplayString("Product: ");
            LCD_DisplayString(products[i].name);

           
            sprintf(price_str, "%.2f", products[i].price);
            LCD_DisplayString("\nPrice: ");
            LCD_DisplayString(price_str);

            total_price += products[i].price;
            sprintf(price_str, "%.2f", total_price);
            LCD_DisplayString("\nTotal: ");
            LCD_DisplayString(price_str);

            UART0_SendString("\nProduct: ");
            UART0_SendString(products[i].name);
            UART0_SendString("\nPrice: ");
            UART0_SendString(price_str);
            UART0_SendString("\nTotal: ");
            UART0_SendString(price_str);
            return;
        }
    }
    LCD_Clear();
    LCD_DisplayString("Unknown RFID Tag");
    UART0_SendString("Unknown RFID Tag\n");
}

// LCD functions
void LCD_Init(void) {
    IO0DIR |= 0x000000FC;  // Set P0.2 to P0.7 as output (data lines)
    IO1DIR |= 0x03000000;  // Set P1.24 and P1.25 as output (control lines)
    LCD_Command(0x38);     // 2 lines, 5x7 matrix
    LCD_Command(0x0C);     // Display ON, cursor OFF
    LCD_Command(0x06);     // Auto increment cursor
    LCD_Command(0x01);     // Clear display
}

void LCD_Command(unsigned char command) {
    IO0PIN = (IO0PIN & 0xFFFFFF03) | (command & 0xFC);  // Send command to data lines
    IO1CLR = 0x01000000;   // RS = 0 for command
    IO1SET = 0x02000000;   // Enable = 1
    delay(500);
    IO1CLR = 0x02000000;   // Enable = 0
}

void LCD_Data(unsigned char data) {
    IO0PIN = (IO0PIN & 0xFFFFFF03) | (data & 0xFC);  // Send data to data lines
    IO1SET = 0x01000000;   // RS = 1 for data
    IO1SET = 0x02000000;   // Enable = 1
    delay(500);
    IO1CLR = 0x02000000;   // Enable = 0
}

void LCD_DisplayString(const char *str) {
    while (*str) {
        LCD_Data(*str++);
    }
}

void LCD_Clear(void) {
    LCD_Command(0x01);   // Clear display
}

// Delay function
void delay(unsigned int count) {
	   int i;
    for (i = 0; i < count * 1000; i++);
}

// Main function
int main(void) {
    char rfid_buffer[RFID_TAG_LENGTH + 1];  // Buffer to store received RFID tag (12 characters + null terminator)
    char received_char;
    int index = 0;

    UART0_Init();    // Initialize UART0 for communication
    LCD_Init();      // Initialize the LCD

    while (1) {
        received_char = UART0_ReceiveChar();  // Receive RFID tag character by character
        if (received_char != '\n' && index < RFID_TAG_LENGTH) {
            rfid_buffer[index++] = received_char;
        } else if (received_char == '\n') {
            rfid_buffer[index] = '\0';  // Null-terminate the string
            check_and_add_product(rfid_buffer);  // Check and add product
            index = 0;  // Reset the buffer index for the next tag
        }
    }
}
