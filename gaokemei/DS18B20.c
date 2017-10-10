/*
 * DS18B20.c
 *
 *  Created on: 2017年9月28日
 *      Author: XJ
 */


/*
 * ds18b20.c
 *
 *  Created on: 2017年8月24日
 *      Author: XJ
 *
 *
 *
 *  DS18B20控制逻辑：
 *      1.初始化
 *      2.写ROM控制命令
 *      3.写DS18B20控制指令
 *      然后就会可以按照发送的指令去读想要的数据
 *      每次操控都要执行以上3步操作
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
 * 启动线程，第一次初始化DS18B20
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
 * 初始化DS18B20
 * */
int init_ds18b20( )
{
    MicoGpioInitialize(GPIO_DQ, OUTPUT_PUSH_PULL);
    ds1820_log("begin init");
    int i = 0;
    MicoGpioOutputLow(GPIO_DQ);//拉低电平
    MicoNanosendDelay(650000);//等待480us-960us
    MicoGpioOutputHigh(GPIO_DQ);//拉高电平
    MicoGpioInitialize(GPIO_DQ, INPUT_PULL_UP);//释放总线(让18b20去控制)
    while(MicoGpioInputGet(GPIO_DQ))//一定能时间内检测到低电平即为初始化成功
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
 * 往DS18B20中写入一个字节的命令
 * **/
void writeDs18b20( unsigned char address )
{
    unsigned int i = 0, j = 0;
    for ( i = 0; i < 8; i++ )//一次写一个位
    {
        MicoGpioInitialize(GPIO_DQ, OUTPUT_PUSH_PULL);
        MicoGpioOutputLow( GPIO_DQ );//拉低电平，通知18b20开始写了
        MicoNanosendDelay(2000);
        if ( address & 0x01 )
        {
            MicoGpioOutputHigh( GPIO_DQ );//高电平就是写1
        }
        else
        {
            MicoGpioOutputLow( GPIO_DQ );//低电平就是写0
        }
        MicoNanosendDelay( 70000 );//延迟60us，一个写周期为60us-120us
        MicoGpioOutputHigh( GPIO_DQ );//写周期完毕，拉高电平
        MicoGpioInitialize(GPIO_DQ, INPUT_PULL_UP);
        address >>= 1;
    }
}

/**
 * 从DS18B20中读一个字节数据
 * */
unsigned int readDs18b20( )
{
    unsigned int byte = 0, bi = 0;
    unsigned int i = 0, j = 0;
    for ( i = 0; i < 8; i++ )//每次读一个位
    {
        MicoGpioInitialize(GPIO_DQ, OUTPUT_PUSH_PULL);
        MicoGpioOutputLow(GPIO_DQ);//拉低电平，告诉18B20我要开始读了
        MicoNanosendDelay(2000);
        MicoGpioOutputHigh(GPIO_DQ);
        MicoGpioInitialize(GPIO_DQ, INPUT_PULL_UP);
        MicoNanosendDelay(3000);
        bi = MicoGpioInputGet(GPIO_DQ);
        byte = (byte >> 1) | (bi << 7);
        MicoNanosendDelay(70000);//一个读周期也是最少为60us
    }
    ds1820_log("byte is %d", byte);
    return byte;
}
int getDs18b20Temp()
{
    float x;
    unsigned int temp =0 ;
    unsigned int tmh, tml;
    changeTemp( ); //先写入转换命令
    readTempCom( ); //然后等待转换完后发送读取温度命令
    tml = readDs18b20( );  //读取温度值共16位，先读低字节
    tmh = readDs18b20( );  //再读高字节
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
    writeDs18b20( 0xcc );     //跳过ROM操作命令
    writeDs18b20( 0x44 );   //温度转换命令
    msleep( 500 );//这个时间一定不能太短
}
void readTempCom()
{
    init_ds18b20( );
    msleep(1);
    writeDs18b20( 0xcc );  //跳过ROM操作命令
    writeDs18b20( 0xbe );  //发送读取温度命令
}











