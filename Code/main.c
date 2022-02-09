/*-----------------------------------------------------------------------------
					Sieci i Systemy teleinformatyczne - projekt
					Generator DTMF
					autor: Michał Tomacha, Krzysztof Półchłopek, Konrad Konewko
					wersja: 1.0
------------------------------------------------------------------------------*/
					
#include "MKL05Z4.h"
#include "DAC.h"
#include "pit.h"
#include "frdm_bsp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define COLUMN_1 8 // PTA 8
#define COLUMN_2 7 // PTA 7
#define COLUMN_3 6 // PTA 6
#define COLUMN_4 5 // PTA 5

#define ROW_1 12 	// PTA 12
#define ROW_2 11 	// PTA 11
#define ROW_3 10 	// PTA 10
#define ROW_4 9 	// PTA 9

// wyj PTB 1

static const uint16_t sine[]={0x1ec,0x239,0x284,0x2cb,0x30d,0x348,0x37a,0x3a2,
	0x3c0,0x3d2,0x3d8,0x3d2,0x3c0,0x3a2,0x37a,0x348,0x30d,0x2cb,0x284,0x239,
	0x1ec,0x19f,0x154,0x10d,0xcb,0x90,0x5e,0x36,0x18,
0x6,0x0,0x6,0x18,0x36,0x5e,0x90,0xcb,0x10d,0x154,0x19f};

static int x = 0, y = 0;// tablica próbek sinusa

void PIT_IRQHandler(void);

int main (void)
{
	uint32_t rows[4] = {BUS_CLOCK/49100,BUS_CLOCK/54400,BUS_CLOCK/60200,BUS_CLOCK/66500};
	uint32_t cols[4] = {BUS_CLOCK/28900,BUS_CLOCK/31900,BUS_CLOCK/35200,BUS_CLOCK/39000};
	DAC_Init();								// Inicjalizacja przetwornika C/A
	DAC_Load_Trig(0xff);			// Wyzwolenie prztwornika C/A wartością początkową
	PIT_Init();								// Inicjalizacja licznika PIT0
	
	// konfiguracja klawiatury
	SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK; 
	
	PORTA->PCR[ROW_1] |= PORT_PCR_MUX(1);
  PORTA->PCR[ROW_2] |= PORT_PCR_MUX(1);
  PORTA->PCR[ROW_3] |= PORT_PCR_MUX(1);
  PORTA->PCR[ROW_4] |= PORT_PCR_MUX(1);
	
  PORTA->PCR[COLUMN_1] |= PORT_PCR_MUX(1);
  PORTA->PCR[COLUMN_2] |= PORT_PCR_MUX(1);
  PORTA->PCR[COLUMN_3] |= PORT_PCR_MUX(1);
  PORTA->PCR[COLUMN_4] |= PORT_PCR_MUX(1);
	
  PORTA->PCR[COLUMN_1] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK; 
  PORTA->PCR[COLUMN_2] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
  PORTA->PCR[COLUMN_3] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
  PORTA->PCR[COLUMN_4] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;

  PTA->PDDR |= (1<<ROW_1); 
  PTA->PDDR |= (1<<ROW_2);
  PTA->PDDR |= (1<<ROW_3);
  PTA->PDDR |= (1<<ROW_4);

  PTA->PSOR |= (1<<ROW_1); 
  PTA->PSOR |= (1<<ROW_2); 
  PTA->PSOR |= (1<<ROW_3); 
  PTA->PSOR |= (1<<ROW_4);
	// koniec konfiguracji klawiatury
	
	while(1)				
	{
			for(unsigned int i = 0; i < 4 ; i++){ //przeszukanie rzedow
            PTA->PCOR |= (1<<(9 + i)); //ustawienie wyjscia na 0
            for(unsigned int j = 1; j < 5 ; j++) { //przeszukanie kolumn
                if( ( PTA->PDIR & (1<<(j + 4)) ) == 0 ){ 
                    PIT->CHANNEL[0].LDVAL = PIT_LDVAL_TSV(cols[j-1]);                    // Zmiana czestotlowosci przerwan
                    PIT->CHANNEL[1].LDVAL = PIT_LDVAL_TSV(rows[i]);                    // Zmiana czestotlowosci przerwan
                    PIT->CHANNEL[0].TCTRL = PIT_TCTRL_TEN_MASK | PIT_TCTRL_TIE_MASK;        // Odblokowanie przerwania i wystartowanie licznika
                    PIT->CHANNEL[1].TCTRL = PIT_TCTRL_TEN_MASK | PIT_TCTRL_TIE_MASK;        // Odblokowanie przerwania i wystartowanie licznika

                    while((PTA->PDIR & (1<<(j + 4))) == 0); // debouncing oraz ignorowanie przytrzymania przycisku
                    PIT->CHANNEL[0].TCTRL ^= PIT_TCTRL_TEN_MASK | PIT_TCTRL_TIE_MASK;        // Zatrzymanie przerwania i licznika
                    PIT->CHANNEL[1].TCTRL ^= PIT_TCTRL_TEN_MASK | PIT_TCTRL_TIE_MASK;        // Zatrzymanie przerwania i licznika
                    DAC_Load_Trig((uint16_t)0x3D8);

            }
        } 
        PTA->PSOR |= (1<<(9 + i)); //ustawienie wyjscia na 1
    }    
	}
}

void PIT_IRQHandler() // obsługa przerwania PIT
{
	if (x>39)
		x=0;
	
	if (y>39)
		y=0;	

	DAC_Load_Trig((uint16_t)(sine[x]+sine[y]));	// wypisanie wartości na wyjściu przetwornika
	
	if(PIT->CHANNEL[0].TFLG == (1<<PIT_TFLG_TIF_SHIFT))		
		x++;
	
	if(PIT->CHANNEL[1].TFLG == (1<<PIT_TFLG_TIF_SHIFT))
		y++;
	
	PIT->CHANNEL[0].TFLG = PIT_TFLG_TIF_MASK;		// Skasuj flagę żądania przerwania
	PIT->CHANNEL[1].TFLG = PIT_TFLG_TIF_MASK;		// Skasuj flagę żądania przerwania		
}
