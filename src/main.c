
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/delay.h>
#include <stdio.h>

// Function to initialize UART
void uart_init(unsigned int baud)
{
    unsigned int ubrr = F_CPU / 16 / baud - 1;
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (1 << USBS0) | (3 << UCSZ00);
}

// Function to send a character over UART
int uart_putchar(char c, FILE *stream)
{
    if (c == '\n')
    {
        uart_putchar('\r', stream);
    }
    while (!(UCSR0A & (1 << UDRE0)))
        ;
    UDR0 = c;
    return 0;
}

// Create a FILE structure to use with printf
FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);

// Function to initialize Timer3 for Fast PWM mode
void Timer3_FastPWM_Init(uint16_t frequency, uint8_t duty_cycle) {
    // Set Fast PWM Mode 14 (ICR3 as TOP)
    TCCR3A = (1 << WGM31) | (1 << COM3A1);
    TCCR3B = (1 << WGM33) | (1 << WGM32) | (1 << CS31); // Prescaler 8

    // Calculate and set ICR3 (TOP)
    uint32_t top = (16000000UL / (8 * frequency)) - 1;
    ICR3 = top;

    // Set OCR3A based on duty cycle
    OCR3A = (top * duty_cycle) / 100;

    // Enable global interrupts (if needed)
    sei();
}

// Function to set new frequency for Timer3
void setPWM3Frequency(uint16_t new_freq) {
    cli(); // Disable interrupts
    uint32_t top = (16000000UL / (8 * new_freq)) - 1;
    ICR3 = top;
    sei(); // Re-enable interrupts
}

// Function to set new duty cycle for Timer3
void setPWM3DutyCycle(uint8_t new_duty) {
    cli(); // Disable interrupts
    OCR3A = (ICR3 * new_duty) / 100;
    sei(); // Re-enable interrupts
}

int main() {

    // Initialize UART with baud rate 9600
    uart_init(9600);

    // Redirect stdout to UART
    stdout = &uart_output;

    // Now you can use printf
    printf("Hello World\n");

    // Set PE3 (OC3A) as output for PWM signal (CLK), PE4 = (START(L)/STOP(H)), PE5 = (CW(H)/CCW(L))
    DDRE |= (1 << PE3) | (1 << PE4) | (1 << PE5);
    // Set PE4 = (START(L)/STOP(H)), PE5 = (CW(H)/CCW(L)) pins to HIGH
    PORTE |= (1 << PINE4) | (1 << PINE5);

    // Initialize Timer3 for Fast PWM with 500 Hz frequency and 50% duty cycle
    uint16_t frequency = 500;
    uint8_t duty_cycle = 90;
    Timer3_FastPWM_Init(frequency, duty_cycle);

    while (1) {
        // Print the current values of OCR3A, ICR3, frequency and duty cycle
        printf("OCR3A: %d\n", OCR3A);
        printf("ICR3: %d\n", ICR3);
        printf("frequency: %d\n", frequency);
        printf("duty_cycle: %d\n", duty_cycle);

        if (PINE & (1 << PINE4)) {
            // Pin is HIGH
            printf("PE4 = HIGH. Motor has been stopped!\n");
            _delay_ms(1000);

            // Start the motor, set PINE4 to LOW
            PORTE &= ~(1 << PINE4);
            printf("PE4 = LOW. Motor has been started!\n");
        } else {
            // Pin is LOW
            printf("PE4 = LOW. Motor is rotating!\n");
        }

        _delay_ms(5000);

        // Increase frequency by 100 Hz
        if (frequency < 3000) {
            frequency += 100;
            setPWM3Frequency(frequency);

            // Decrease duty cycle by 2%
            while (OCR3A > ICR3)
            {
                /* code */
                duty_cycle -= 2;
                setPWM3DutyCycle(duty_cycle);
            }
        }
        else
        {
            // Stop the motor, set PINE4 to HIGH
            PORTE ^= (1 << PINE4);
            printf("PE4 = HIGH. Motor has been stopped!\n");
            frequency = 500;
            duty_cycle = 90;
            Timer3_FastPWM_Init(frequency, duty_cycle);

            // Change the direction of the motor
            PORTE ^= (1 << PINE5);
            printf("PE5 = LOW. Motor has been changed direction!\n");
            _delay_ms(3000);
        }
    }
}



