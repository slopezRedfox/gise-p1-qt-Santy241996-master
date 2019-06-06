#ifndef RPCCOMMANDS_H
#define RPCCOMMANDS_H

//Códigos de los comandos y definicion de parametros para el protocolo RPC

// El estudiante debe añadir aqui cada nuevo comando que implemente. IMPORTANTE el orden de los comandos
// debe SER EL MISMO aqui, y en el codigo equivalente en la parte del microcontrolador (Code Composer)
typedef enum {
    COMMAND_REJECTED,
    COMMAND_PING,
    COMMAND_LED_GPIO,
    COMMAND_LED_PWM_BRIGHTNESS,
    COMMAND_MODO,
    COMMAND_RGB,
    COMMAND_ESTATUS_PERIFERICOS,
    COMMAND_INTERRUPCION,
    COMMAND_ADC_SAMPLE,
    COMMAND_ADC_GRAF,
    COMMAND_SENSORRGB,
    COMMAND_SENSORPROX,
    COMMAND_SENSORGEST,

    COMMAND_SENSORGEST_SEND_QT,
    COMMAND_ADC_SAMPLE12,
    COMMAND_ADC_SAMPLE8,
    COMMAND_ADC_SAMPLE8_s,
    COMMAND_SENSORPROX_SEND,
    COMMAND_SENSORGEST_SEND,
    COMMAND_SENSOR_SEND,
    COMMAND_SENSOR_SEND_QT
    //etc, etc...
} commandTypes;

//Estructuras relacionadas con los parametros de los comandos. El estuadiante debera crear las
// estructuras adecuadas a los comandos usados, y asegurarse de su compatibilidad en ambos extremos

#pragma pack(1)	//Cambia el alineamiento de datos en memoria a 1 byte.

typedef struct {
    uint8_t command;
} PARAMETERS_COMMAND_REJECTED;

typedef union{
    struct {
         uint8_t padding:1;
         uint8_t red:1;
         uint8_t blue:1;
         uint8_t green:1;
    } leds;
    uint8_t value;
} PARAMETERS_LED_GPIO;

typedef struct {
    float rIntensity;
} PARAMETERS_LED_PWM_BRIGHTNESS;

typedef struct {
    bool Mode_Cambio;
} PARAMETERS_MODO;

typedef struct {
    int rojo;
    int verde;
    int azul;
} PARAMETERS_RGB;

typedef struct{
    uint8_t value;
} PARAMETERS_ESTATUS_PERIFERICOS;


typedef struct {
    bool Interrupcion;
} PARAMETERS_INTERRUPCION;

typedef struct
{
    bool Interrupcion;
    int frecuencia;
    uint8_t NumMuestras;
} PARAMETERS_ADC_GRAF;

typedef struct
{
    uint16_t chan1;
    uint16_t chan2;
    uint16_t chan3;
    uint16_t chan4;
} PARAMETERS_ADC_SAMPLE;

typedef struct
{
    uint8_t chan1;
    uint8_t chan2;
    uint8_t chan3;
    uint8_t chan4;
} PARAMETERS_ADC_SAMPLE8_s;

typedef struct{
    uint16_t chan1[4];
    uint16_t chan2[4];
    uint16_t chan3[4];
    uint16_t chan4[4];
} PARAMETERS_ADC_SAMPLE12;

typedef struct{
    uint8_t chan1[8];
    uint8_t chan2[8];
    uint8_t chan3[8];
    uint8_t chan4[8];
} PARAMETERS_ADC_SAMPLE8;

typedef struct{
    uint16_t R;
    uint16_t G;
    uint16_t B;
    uint16_t I;
} PARAMETERS_SENSORRGB;

typedef struct{
    uint8_t Distancia;
    uint8_t Interrupcion_on;
} PARAMETERS_SENSORPROX;

typedef struct{
    uint8_t Interrupcion_on;
    uint8_t Data_Qt;
} PARAMETERS_SENSORGEST;

typedef struct{
    uint8_t ON;
    uint8_t proximity_data;
} PARAMETERS_SENSORPROX_SEND;

typedef struct{
    uint8_t GEST;
} PARAMETERS_SENSORGEST_SEND;

typedef struct{
    uint8_t GEST;
    uint8_t ON;
    uint8_t proximity_data;
} PARAMETERS_SENSOR_SEND;

typedef struct{
    uint8_t u_data[8];
    uint8_t d_data[8];
    uint8_t l_data[8];
    uint8_t r_data[8];

} PARAMETERS_SENSORGEST_SEND_QT;

typedef struct{
    uint8_t u_data[8];
    uint8_t d_data[8];
    uint8_t l_data[8];
    uint8_t r_data[8];

    uint8_t ON;
    uint8_t proximity_data;
} PARAMETERS_SENSOR_SEND_QT;

#pragma pack()	//...Pero solo para los comandos que voy a intercambiar, no para el resto.

#endif // RPCCOMMANDS_H
