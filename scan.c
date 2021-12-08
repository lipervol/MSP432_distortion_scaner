#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <oled.h>

#define Times 1000

const Timer_A_UpModeConfig upModeConfig =
{
        TIMER_A_CLOCKSOURCE_SMCLK,
        TIMER_A_CLOCKSOURCE_DIVIDER_1,
        4,



        TIMER_A_TAIE_INTERRUPT_DISABLE,
        TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE,
        TIMER_A_DO_CLEAR
};

const Timer_A_CompareModeConfig compareConfig =
{
        TIMER_A_CAPTURECOMPARE_REGISTER_1,
        TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE,
        TIMER_A_OUTPUTMODE_SET_RESET,
        2
};

static volatile uint_fast16_t resultsBuffer[Times];
static volatile uint_fast16_t resultsBuffer0[Times];
static volatile uint_fast16_t resultsBuffer1[Times];
static volatile uint16_t resPos;
uint16_t n;
uint8_t Over_Flag=0;
uint8_t type=0;

uint16_t Trans(uint8_t data){
    return ((data>>4)<<8)+((data&0x0f)<<4);
}



void DMA_Send(uint16_t data){
    uint16_t i;
    for(i=0;i<1;i++);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN5);
    for(i=0;i<16;i++){
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P1,GPIO_PIN5);
        if(data&0x8000) MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P1,GPIO_PIN6);
        else MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1,GPIO_PIN6);
        data<<=1;
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1,GPIO_PIN5);
    }
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3,GPIO_PIN5);
}

int IsOk(uint_fast16_t a,uint_fast16_t b,uint16_t d){
    if((a>b+d)||(b>a+d)){
        return 1;
    }
    else return 0;
}



void DMA_Init(void){
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_PJ,GPIO_PIN3 | GPIO_PIN2, GPIO_PRIMARY_MODULE_FUNCTION);

    CS_setExternalClockSourceFrequency(32000,48000000);
    MAP_PCM_setCoreVoltageLevel(PCM_VCORE1);
    MAP_FlashCtl_setWaitState(FLASH_BANK0, 1);
    MAP_FlashCtl_setWaitState(FLASH_BANK1, 1);
    CS_startHFXT(false);

    MAP_CS_initClockSignal(CS_MCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);

    MAP_GPIO_setAsOutputPin(GPIO_PORT_P1,GPIO_PIN5|GPIO_PIN6);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P3,GPIO_PIN5);
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3,GPIO_PIN5);
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P1,GPIO_PIN5|GPIO_PIN6);
}

int ii=0;
uint8_t sign=0;
uint16_t a1,a2,a3;

uint16_t FindOver(volatile uint_fast16_t Buffer[],int Timess){
    uint16_t oo,flag=0;
    for(oo=2;n<Timess;oo++){
    if((Buffer[oo]<Buffer[oo-1]+5)&&(Buffer[oo-1]<Buffer[oo]+5)) {
              if(flag>=15) break;
              else flag++;
            }
            else flag=0;
     }
     return oo=oo-flag;
}

uint16_t resPos0=0;
uint16_t resPos1=0;

void Handler_Out(volatile uint_fast16_t Buffer[],int an,uint16_t resn){
    Buffer[0]=(Buffer[an-2]+Buffer[1])/2;
    Buffer[an-2]=(Buffer[0]+Buffer[an-3])/2;
    Buffer[an-1]=(Buffer[an-2]+Buffer[an])/2;
    Buffer[an]=(Buffer[an-1]+Buffer[0])/2;

    for(resn=0;resn<Times;resn++) {
            Buffer[resn]=Trans(Buffer[resn]);
        }
}

int main(void)
{
    MAP_WDT_A_holdTimer();
    resPos = 0;
    DMA_Init();
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_PJ,GPIO_PIN0 | GPIO_PIN1, GPIO_PRIMARY_MODULE_FUNCTION);
    MAP_CS_initClockSignal(CS_MCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_1_5);
    CS_initClockSignal(CS_MCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_1 );
    CS_initClockSignal(CS_HSMCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_4 );
    CS_initClockSignal(CS_SMCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_1 );
    CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1 );
    MAP_FlashCtl_setWaitState(FLASH_BANK0, 3);
    MAP_FlashCtl_setWaitState(FLASH_BANK1, 3);

    MAP_ADC14_enableModule();
    MAP_ADC14_initModule(ADC_CLOCKSOURCE_MCLK, ADC_PREDIVIDER_1, ADC_DIVIDER_1,
                0);
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P5, GPIO_PIN5,
    GPIO_TERTIARY_MODULE_FUNCTION);
    MAP_ADC14_configureSingleSampleMode(ADC_MEM0, true);
    MAP_ADC14_configureConversionMemory(ADC_MEM0, ADC_VREFPOS_AVCC_VREFNEG_VSS,
    ADC_INPUT_A0, false);
    ADC14_setSampleHoldTrigger(ADC_PULSE_WIDTH_4,ADC_PULSE_WIDTH_4);
    MAP_Timer_A_configureUpMode(TIMER_A0_BASE, &upModeConfig);
    MAP_Timer_A_initCompare(TIMER_A0_BASE, &compareConfig);
    MAP_ADC14_setSampleHoldTrigger(ADC_TRIGGER_SOURCE1, false);
    MAP_ADC14_enableInterrupt(ADC_INT0);
    MAP_ADC14_enableConversion();
    MAP_Interrupt_enableInterrupt(INT_ADC14);
    MAP_Interrupt_enableMaster();
    MAP_Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);

    MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1,GPIO_PIN4);
    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P1,GPIO_PIN4);
    MAP_GPIO_enableInterrupt(GPIO_PORT_P1,GPIO_PIN4);
    MAP_Interrupt_enableInterrupt(INT_PORT1);

    a1=FindOver(resultsBuffer,Times);
    a2=FindOver(resultsBuffer0,Times);
    a3=FindOver(resultsBuffer1,Times);
    Handler_Out(resultsBuffer,a1,resPos);
    Handler_Out(resultsBuffer0,a2,resPos0);
    Handler_Out(resultsBuffer1,a3,resPos1);

    while(1){
        if(type==0){
            if(resPos>a1-2) resPos=0;
            DMA_Send(resultsBuffer[resPos]);
            resPos++;
                }
        if(type==1){
                    if(resPos0>a2-2) resPos0=0;
                    DMA_Send(resultsBuffer0[resPos0]);
                    resPos0++;
                }
        if(type==2){
                    if(resPos1>a3-2) resPos1=0;
                    DMA_Send(resultsBuffer1[resPos1]);
                    resPos1++;
                }
    }


}

void ADC14_IRQHandler(void)
{
    uint64_t status;
    status = MAP_ADC14_getEnabledInterruptStatus();
    if (status & ADC_INT0)
    {
        resPos++;
        if(ii==0){
            resultsBuffer[resPos] = MAP_ADC14_getResult(ADC_MEM0)*144/16384;
            if(resPos>Times) {
                resPos=0;sign=0;ii++;
            }
        }
        else if(ii==1){
            resultsBuffer0[resPos] = MAP_ADC14_getResult(ADC_MEM0)*144/16384;
            if(resPos>Times) {
                resPos=0;sign=0;ii++;
            }
        }
        else if(ii==2){
            resultsBuffer1[resPos] = MAP_ADC14_getResult(ADC_MEM0)*144/16384;
            if(resPos>Times) {
                resPos=0;
                MAP_Interrupt_disableInterrupt(INT_ADC14);
                        }
        }
       if(resPos>1&&!sign){
            if(ii==0){
                if(IsOk(resultsBuffer[resPos],resultsBuffer[resPos-1],50)) sign=1;
                else resPos=1;
            }
            if(ii==1){
                if(IsOk(resultsBuffer0[resPos],resultsBuffer0[resPos-1],50)) sign=1;
                else resPos=1;
            }
            if(ii==2){
                if(IsOk(resultsBuffer1[resPos],resultsBuffer1[resPos-1],50)) sign=1;
                else resPos=1;
            }
        }
    }
    MAP_ADC14_clearInterruptFlag(status);

}

void PORT1_IRQHandler(void){
    uint64_t status;
    status=MAP_GPIO_getEnabledInterruptStatus(GPIO_PORT_P1);
    if(status&GPIO_PIN4){
            MAP_SysTick_disableModule();
            MAP_SysTick_setPeriod(15000);
            MAP_SysTick_enableModule();
            while(MAP_SysTick_getValue()<1024);
            MAP_SysTick_disableModule();
            if(!MAP_GPIO_getInputPinValue(GPIO_PORT_P1,GPIO_PIN4)){
                type++;
                if(type>2) type=0;
            }
            MAP_SysTick_setPeriod(15000);
            MAP_SysTick_enableModule();
            while(MAP_SysTick_getValue()<1024);
            MAP_SysTick_disableModule();
            MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1,GPIO_PIN4);
        }
        MAP_GPIO_clearInterruptFlag(GPIO_PORT_P1,status);
}

