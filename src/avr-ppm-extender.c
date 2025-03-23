/*
 *
 * avr-ppm-extender.c
 *
 * Copyright (C) 2025  Barnard33
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * This code is only intended to be used for educational purposes.
 * It is not production stable and must under no circumstance be used
 * in any kind of radio controlled machinery, e.g., planes, cars, boats, etc.
 *
 * Created: 2025-03-23 17:01
 *
 */

#ifndef F_CPU
#error F_CPU not defined in make file or as compiler command argument
#endif

#ifndef NOP
#define NOP asm("NOP")
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>

#include "ppm_ticks.h"

#define SERVO_IN PB1
#define SERVO_OUT PB4

volatile enum uint8_t { MODE_SERVO_IN, MODE_SERVO_OUT } timer_mode;

volatile uint8_t servo_in_latch = 0;
volatile uint8_t servo_out_latch = 0;

volatile uint8_t timer_overflows = 0;
volatile uint8_t expected_timer_overflows = 0;

inline void setup(void)
{
    DDRB |= (1 << SERVO_OUT);   // define as output pin
    DDRB &= ~(1 << SERVO_IN);   // define as input pin
    PORTB &= ~(1 << SERVO_IN);  // disable pull-up

    // CPU starts automatically with prescaler division factor 8
    // Instead of programming fuses, the division factor is set by software
    uint8_t clkpr1 = 0;
    clkpr1 |= (1 << CLKPCE);
    CLKPR = clkpr1; // init prescaler change procedure
    CLKPR = 0; // change CPU clock prescaler to division factor 1
}

inline void enable_timer_isr_servo_out(void)
{
    TIMSK0 |= (1 << OCIE0A) | (1 << TOIE0); // enable timer interrupts on compare match and on timer overflow
}

inline void disable_timer_isr_servo_out(void)
{
    TIMSK0 &= ~((1 << OCIE0A) | (1 << TOIE0)); // disable timer interrupts on compare match and on timer overflow
}

inline void enable_timer_isr_servo_in(void)
{
    TIMSK0 |= (1 << TOIE0); // enable timer interrupt on timer overflow
}

inline void disable_timer_isr_servo_in(void)
{
    TIMSK0 &= ~(1 << TOIE0); // disable timer interrupt on timer overflow
}

inline void start_timer(void)
{
    // set timer clock source prescaler to clk_io/8 and start the timer
    TCCR0B |= (1 << CS01);
}

inline void stop_timer(void)
{
    TCCR0B &= ~(1 << CS01);
}

inline void enable_int0_isr_servo_in(void)
{
    GIMSK |= (1 << INT0);
}

inline void disable_int0_isr_servo_in(void)
{
    GIMSK &= ~(1 << INT0);
}

inline void listen_on_rising_edge_of_servo_in(void)
{
    MCUCR |= (1 << ISC00) | (1 << ISC01);
}

inline void listen_on_falling_edge_of_servo_in(void)
{
    MCUCR &= ~(1 << ISC00);
    MCUCR |= (1 << ISC01);
}

inline void set_servo_pin_high(void)
{
    PORTB |= (1 << SERVO_OUT);
}

inline void set_servo_pin_low(void)
{
    PORTB &= ~(1 << SERVO_OUT);
}

/**
 * Interrupt handler for timer overflow.
 */
ISR(TIM0_OVF_vect)
{
    switch(timer_mode) {
    case MODE_SERVO_IN:
        timer_overflows++;
        break;
    case MODE_SERVO_OUT:
        if(timer_overflows < expected_timer_overflows) {
            timer_overflows++;
        }
        break;
    }
}

/**
 * Interrupt handler for timer compare match.
 */
ISR(TIM0_COMPA_vect)
{
    if(timer_overflows >= expected_timer_overflows) {
        set_servo_pin_low();
        stop_timer();
        servo_out_latch++;
    }
}

/**
 * Interrupt handler for servo signal input pin.
 */
ISR(INT0_vect)
{
    if(MCUCR & (1 << ISC00)) {
        // on rising edge
        start_timer();
        listen_on_falling_edge_of_servo_in();
    } else {
        // on falling edge
        stop_timer();
        servo_in_latch++;
    }
}

/**
 * Scales the given value by 2.666
 */
inline uint16_t scale(const uint16_t value)
{
    return (value * 2) + 2 * (value / 3);
}

/**
 * Converts the [timer_ticks] to necessary timer ticks for 270 degree.
 */
inline uint16_t expand_timer_ticks(const uint16_t timer_ticks)
{
    const uint16_t h = MID_TICKS - IN_MIN_TICKS;
    const uint16_t p = timer_ticks - IN_MIN_TICKS;
    uint16_t expanded = MID_TICKS;
    if(p > h) {
        expanded = MID_TICKS + scale(p - h);
    } else if(p < h) {
        expanded = MID_TICKS - scale(h - p);
    }

    return expanded;
}

#ifndef __OUT_TEST__
inline void main_loop(void)
{
    while(1) {
        timer_mode = MODE_SERVO_IN;
        TCNT0 = 0;
        timer_overflows = 0;
        servo_in_latch = 0;
        enable_int0_isr_servo_in();
        listen_on_rising_edge_of_servo_in();
        enable_timer_isr_servo_in();
        sei();
        while(servo_in_latch < 1) NOP; //active wait
        cli();
        disable_timer_isr_servo_in();
        disable_int0_isr_servo_in();

        // calc real timer tick value
        uint16_t in_timer_ticks = (timer_overflows << 8) | TCNT0;

        // check if signal is valid - if not proceed with next signal
        if(in_timer_ticks < IN_MIN_TICKS || in_timer_ticks > IN_MAX_TICKS) continue;

        // convert signal to 270 degree signal
        uint16_t out_timer_ticks = expand_timer_ticks(in_timer_ticks);

        // sanitize expanded signal to allowed max ratings
        if(out_timer_ticks < OUT_MIN_TICKS) out_timer_ticks = OUT_MIN_TICKS;
        if(out_timer_ticks > OUT_MAX_TICKS) out_timer_ticks = OUT_MAX_TICKS;

        // calc expected timer overflows as high byte of the timer ticks
        expected_timer_overflows = (uint8_t)(out_timer_ticks >> 8);
        // calc output compare value as low byte of the timer ticks
        OCR0A = (uint8_t)(out_timer_ticks & 0x00FF);

        timer_mode = MODE_SERVO_OUT;
        enable_timer_isr_servo_out();
        TCNT0 = 0;
        timer_overflows = 0;
        servo_out_latch = 0;
        sei();
        set_servo_pin_high();
        start_timer();
        while(servo_out_latch < 1) NOP; // active wait
        cli();
        disable_timer_isr_servo_out();
    }
}
#endif

#ifdef __OUT_TEST__
inline static void main_loop(void)
{
    enum uint8_t {UP, DOWN} direction = UP;
    uint16_t in_timer_ticks = MID_TICKS;

    while(1) {
        if(direction == UP && in_timer_ticks >= IN_MAX_TICKS) {
            direction = DOWN;
        }
        if(direction == DOWN && in_timer_ticks <= IN_MIN_TICKS) {
            direction = UP;
        }
        switch(direction) {
        case UP:
            in_timer_ticks += 2;
            break;
        case DOWN:
            in_timer_ticks -= 2;
            break;
        }

        // convert signal to 270 degree signal
        uint16_t out_timer_ticks = expand_timer_ticks(in_timer_ticks);

        // sanitize expanded signal to allowed max ratings
        if(out_timer_ticks < OUT_MIN_TICKS) out_timer_ticks = OUT_MIN_TICKS;
        if(out_timer_ticks > OUT_MAX_TICKS) out_timer_ticks = OUT_MAX_TICKS;

        // calc expected timer overflows as high byte of the timer ticks
        expected_timer_overflows = (uint8_t)(out_timer_ticks >> 8);
        // calc output compare value as low byte of the timer ticks
        OCR0A = (uint8_t)(out_timer_ticks & 0x00FF);

        TCNT0 = 0;
        timer_overflows = 0;
        servo_out_latch = 0;
        enable_timer_isr_servo_out();
        sei();
        set_servo_pin_high();
        start_timer();
        while(servo_out_latch < 1) NOP;
        cli();
        disable_timer_isr_servo_out();

        _delay_ms(19);
    }
}
#endif

int main(void)
{
    setup();
    main_loop();
    return 0;
}
