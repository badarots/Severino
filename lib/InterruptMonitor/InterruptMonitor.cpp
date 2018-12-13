/*
  Morse.cpp - Library for flashing Morse code.
  Created by David A. Mellis, November 2, 2007.
  Released into the public domain.
*/

#include "Arduino.h"
#include "InterruptMonitor.h"

InterruptMonitor::InterruptMonitor(byte input_pin, unsigned long msdelay)
{
    pin = input_pin;
    pinMode(pin, INPUT_PULLUP);

    delay = msdelay;
    flag = do_read = false;
    last_read = digitalRead(pin);
    last_schedule = 0;
}

// função que levanta a flag de mudança de estado
// deve ser chama externamente, a flag gera um agendamendo no proximo update
void InterruptMonitor::raiseFlag() {
    flag = true;
}

int InterruptMonitor::update() {
    // variável que guarda resposta
    // -1: não houve mudanças, 0: pino mudou para low, 1: pino mudou para high
    int resp = -1;

    // agenda leitura atrasada:
    if (flag) {
        last_schedule = millis();
        flag = false;
        do_read = true;
    }
    // executa leitura atrasada
    if (do_read && millis() - last_schedule > delay) {
        do_read = false;
        bool read = digitalRead(pin);

        if (read != last_read) resp = last_read = read;
    }
    return resp;
}
