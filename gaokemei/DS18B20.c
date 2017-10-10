/*
 * DS18B20.c
 *
 *  Created on: 2017��9��28��
 *      Author: XJ
 */


/*
 * ds18b20.c
 *
 *  Created on: 2017��8��24��
 *      Author: XJ
 *
 *
 *
 *  DS18B20�����߼���
 *      1.��ʼ��
 *      2.дROM��������
 *      3.дDS18B20����ָ��
 *      Ȼ��ͻ���԰��շ��͵�ָ��ȥ����Ҫ������
 *      ÿ�βٿض�Ҫִ������3������
 */
//#include "helloworld.h"
#include "mico.h"

#include "alink_device.h"
#define ds1820_log(format, ...)  custom_log("helloworld", format, ##__VA_ARGS__)
#define GPIO_DQ MICO_GPIO_8
int DSIO = 0;
__IO int tempDS = 0;

#define TemperatureData    "{\"Temperature\": { \"value\": \"%d\" }}"
static char temp_data_tx_buffer[128];
int temperature_to_cloud()
{
    sprintf(temp_data_tx_buffer, TemperatureData, tempDS);
    return alink_report(Method_PostData, (char *)temp_data_tx_buffer);
}
/**
 * �����̣߳���һ�γ�ʼ��DS18B20
 * */
void ds18b20_thread()
{
    MicoGpioInitialize(GPIO_DQ, OUTPUT_PUSH_PULL);
//    while(1)
//    {
//        MicoGpioInitialize(GPIO_DQ, OUTPUT_PUSH_PULL);
//        MicoGpioOutputLow(GPIO_DQ);
//        MicoNanosendDelay(6000);
//        MicoGpioOutputHigh(GPIO_DQ);
//        MicoGpioInitialize(GPIO_DQ, INPUT_PULL_UP);
//        msleep(100);
//    }
    int err = 0;
    err = init_ds18b20();
    if(err == 0)
        ds1820_log("ds18b20 init err");
    else
    {
        ds1820_log("ds18b20 init success");
    }

    while(1)
    {
        getDs18b20Temp();
        sleep(3);
        temperature_to_cloud();
    }

}

/*
 * ��ʼ��DS18B20
 * */
int init_ds18b20( )
{
    MicoGpioInitialize(GPIO_DQ, OUTPUT_PUSH_PULL);
    ds1820_log("begin init");
    int i = 0;
    MicoGpioOutputLow(GPIO_DQ);//���͵�ƽ
    MicoNanosendDelay(650000);//�ȴ�480us-960us
    MicoGpioOutputHigh(GPIO_DQ);//���ߵ�ƽ
    MicoGpioInitialize(GPIO_DQ, INPUT_PULL_UP);//�ͷ�����(��18b20ȥ����)
    while(MicoGpioInputGet(GPIO_DQ))//һ����ʱ���ڼ�⵽�͵�ƽ��Ϊ��ʼ���ɹ�
    {
        i++;
        MicoNanosendDelay(2000);
        if(i>500)
        {
            ds1820_log("ds18b20 init err");
            return 0;
        }
    }
    MicoNanosendDelay(300000);
    ds1820_log("ds18b20 init success");
    return 1;
}

/**
 * ��DS18B20��д��һ���ֽڵ�����
 * **/
void writeDs18b20( unsigned char address )
{
    unsigned int i = 0, j = 0;
    for ( i = 0; i < 8; i++ )//һ��дһ��λ
    {
        MicoGpioInitialize(GPIO_DQ, OUTPUT_PUSH_PULL);
        MicoGpioOutputLow( GPIO_DQ );//���͵�ƽ��֪ͨ18b20��ʼд��
        MicoNanosendDelay(2000);
        if ( address & 0x01 )
        {
            MicoGpioOutputHigh( GPIO_DQ );//�ߵ�ƽ����д1
        }
        else
        {
            MicoGpioOutputLow( GPIO_DQ );//�͵�ƽ����д0
        }
        MicoNanosendDelay( 70000 );//�ӳ�60us��һ��д����Ϊ60us-120us
        MicoGpioOutputHigh( GPIO_DQ );//д������ϣ����ߵ�ƽ
        MicoGpioInitialize(GPIO_DQ, INPUT_PULL_UP);
        address >>= 1;
    }
}

/**
 * ��DS18B20�ж�һ���ֽ�����
 * */
unsigned int readDs18b20( )
{
    unsigned int byte = 0, bi = 0;
    unsigned int i = 0, j = 0;
    for ( i = 0; i < 8; i++ )//ÿ�ζ�һ��λ
    {
        MicoGpioInitialize(GPIO_DQ, OUTPUT_PUSH_PULL);
        MicoGpioOutputLow(GPIO_DQ);//���͵�ƽ������18B20��Ҫ��ʼ����
        MicoNanosendDelay(2000);
        MicoGpioOutputHigh(GPIO_DQ);
        MicoGpioInitialize(GPIO_DQ, INPUT_PULL_UP);
        MicoNanosendDelay(3000);
        bi = MicoGpioInputGet(GPIO_DQ);
        byte = (byte >> 1) | (bi << 7);
        MicoNanosendDelay(70000);//һ��������Ҳ������Ϊ60us
    }
    ds1820_log("byte is %d", byte);
    return byte;
}
int getDs18b20Temp()
{
    float x;
    unsigned int temp =0 ;
    unsigned int tmh, tml;
    changeTemp( ); //��д��ת������
    readTempCom( ); //Ȼ��ȴ�ת������Ͷ�ȡ�¶�����
    tml = readDs18b20( );  //��ȡ�¶�ֵ��16λ���ȶ����ֽ�
    tmh = readDs18b20( );  //�ٶ����ֽ�
    ds1820_log("tml is %d", tml);
    ds1820_log("tmh is %d", tmh);
    temp = tmh;
    temp <<= 8;
    ds1820_log("111temp is %d", temp);
    temp |= tml;
    ds1820_log("temp is %d", temp);
    ds1820_log("temp is %u", temp);
    ds1820_log("temp is %x", temp);
    x = temp*0.0625;
    ds1820_log("xxxxxxxxxxx = %f", x);
    tempDS = x;
    return temp;
}
void changeTemp()
{
    init_ds18b20( );
    msleep(1);
    writeDs18b20( 0xcc );     //����ROM��������
    writeDs18b20( 0x44 );   //�¶�ת������
    msleep( 500 );//���ʱ��һ������̫��
}
void readTempCom()
{
    init_ds18b20( );
    msleep(1);
    writeDs18b20( 0xcc );  //����ROM��������
    writeDs18b20( 0xbe );  //���Ͷ�ȡ�¶�����
}











