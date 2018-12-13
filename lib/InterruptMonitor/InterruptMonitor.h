/*
  Morse.h - Library for flashing Morse code.
  Created by David A. Mellis, November 2, 2007.
  Released into the public domain.
*/
#ifndef InterrupMonitor_h
#define InterrupMonitor_h

#include "Arduino.h"

class InterruptMonitor
{
  public:
    // Mètodos públicos
    InterruptMonitor(byte input_pin, unsigned long msdelay);
    void raiseFlag();
    int update();

    // variáveis de configuração
	byte pin;             // numero do pino de entrada
    unsigned long delay;   // atraso em ms entre o agendamento e a leitura


  private:
    // variáveis internas
    volatile bool flag;   // flag que avisa quando há mudança de estado
    bool do_read;         // flag que marca o agendamento de um leitura atrasada
    bool last_read;       // estado da última leitura
    unsigned long last_schedule;  // último agendamento da leitura
};
#endif
