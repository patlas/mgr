#include "stm32f3xx_hal.h"

void advanced_int_init() {
	
	//disable TIMA rep int
	HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].TIMxDIER &= ~(HRTIM_TIM_IT_REP);
	
	//stop TIMC
	HRTIM1->sMasterRegs.MCR &= ~(HRTIM_TIMERID_TIMER_C);
	
	//enable TIMC comp1 int
	HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].TIMxDIER |= (HRTIM_TIM_IT_CMP1);
	
}


void update_phase(uint16_t val) {
	//UPDATE TIMC CMP1 register
	HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].CMP1xR = val;
	//ENABLE TIMA_PRELOAD_IT
	HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].TIMxDIER |= (HRTIM_TIM_IT_REP);
}

//all configuration for this interrupt made in init stage
//void TIMA_PRELOAD_INTERRUPT() {
void HRTIM1_TIMA_IRQHandler(void) {
	
	//check if pending intrrupt is REP
	//if yes do below elsewere ommit
	if( HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].TIMxISR & HRTIM_TIM_IT_REP) {
		//START TIMC
		HRTIM1->sMasterRegs.MCR |= HRTIM_TIMERID_TIMER_C;
		
		//DISABLE TIMA_PRELOAD_INT (flag)
		HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].TIMxDIER &= ~(HRTIM_TIM_IT_REP);
		
		//CLEAR PENDING INT FLAG
		HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].TIMxICR = HRTIM_TIM_IT_REP;
	}
}

//all configuration for this interrupt in init stage - interrupt always on!
//period as long as signal (6.78) period? - maby lower?
//void TIMC_COMP1_INTERRUPT() {
void HRTIM1_TIMC_IRQHandler() {
	
	//CLEAR PENDING INT FLAG
	HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].TIMxICR = HRTIM_TIM_IT_CMP1;
	
	//START TIMB -> will generate signal phase shifted
	HRTIM1->sMasterRegs.MCR |= HRTIM_TIMERID_TIMER_B;
	
	//STOP TIMC -> RESET Timer enable flag
	HRTIM1->sMasterRegs.MCR &= ~(HRTIM_TIMERID_TIMER_C);
}