#include "qtivarpc.h"

//Implementacion de la clase que permite controlar el microcontrolador TIVA mediante el interfaz RPC


#include <QSerialPort>      // Comunicacion por el puerto serie
#include <QSerialPortInfo>  // Comunicacion por el puerto serie

extern "C" {
#include "serialprotocol.h"    // Cabecera de funciones de gestión de tramas; se indica que está en C, ya que QTs
#include "rpc_commands.h"
// se integra en C++, y el C puede dar problemas si no se indica.
}

QTivaRPC::QTivaRPC(QObject *parent) : QObject(parent)
{

    connected=false;

    // Las funciones CONNECT son la base del funcionamiento de QT; conectan dos componentes
    // o elementos del sistema; uno que GENERA UNA SEÑAL; y otro que EJECUTA UNA FUNCION (SLOT) al recibir dicha señal.
    // En el ejemplo se conecta la señal readyRead(), que envía el componente que controla el puerto USB serie (serial),
    // con la propia clase PanelGUI, para que ejecute su funcion readRequest() en respuesta.
    // De esa forma, en cuanto el puerto serie esté preparado para leer, se lanza una petición de datos por el
    // puerto serie.El envío de readyRead por parte de "serial" es automatico, sin necesidad de instrucciones
    // del programador
    connect(&serial, SIGNAL(readyRead()), this, SLOT(processIncommingSerialData()));
}


//Este slot se conecta con la señal readyRead() del puerto serie, que se activa cuando hay algo que leer del puerto serie
//Se encarga de procesar y decodificar los datos que llegan de la TIVA y generar señales en respuesta a algunos de ellos
//Estas señales son capturadas por slots de la clase guipanel en este ejemplo.
void QTivaRPC::processIncommingSerialData()
{
    int StopCharPosition,StartCharPosition,tam;   // Solo uso notacin hungara en los elementos que se van a
    // intercambiar con el micro - para control de tamaño -
    uint8_t *pui8Frame; // Puntero a zona de memoria donde reside la trama recibida
    void *ptrtoparam;
    uint8_t ui8Command; // Para almacenar el comando de la trama entrante


    incommingDataBuffer.append(serial.readAll()); // Añade el contenido del puerto serie USB al array de bytes 'incommingDataBuffer'
    // así vamos acumulando  en el array la información que va llegando

    // Busca la posición del primer byte de fin de trama (0xFD) en el array. Si no estuviera presente,
    // salimos de la funcion, en caso contrario, es que ha llegado al menos una trama.
    // Hay que tener en cuenta que pueden haber llegado varios paquetes juntos.
    StopCharPosition=incommingDataBuffer.indexOf((char)STOP_FRAME_CHAR,0);
    while (StopCharPosition>=0)
    {
        //Ahora buscamos el caracter de inicio correspondiente.
        StartCharPosition=incommingDataBuffer.lastIndexOf((char)START_FRAME_CHAR,0); //Este seria el primer caracter de inicio que va delante...

        if (StartCharPosition<0)
        {
            //En caso de que no lo encuentre, no debo de hacer nada, pero debo vaciar las primeras posiciones hasta STOP_FRAME_CHAR (inclusive)
            incommingDataBuffer.remove(0,StopCharPosition+1);
            LastError=QString("Status:Fallo trozo paquete recibido");
            emit statusChanged(QTivaRPC::FragmentedPacketError,LastError);

        } else
        {
            incommingDataBuffer.remove(0,StartCharPosition); //Si hay datos anteriores al caracter de inicio, son un trozo de trama incompleto. Los tiro.
            tam=StopCharPosition-StartCharPosition+1;//El tamanio de la trama es el numero de bytes desde inicio hasta fin, ambos inclusive.
            if (tam>=MINIMUN_FRAME_SIZE)
            {
                pui8Frame=(uint8_t*)incommingDataBuffer.data(); // Puntero de trama al inicio del array de bytes
                pui8Frame++; //Nos saltamos el caracter de inicio.
                tam-=2; //Descontamos los bytes de inicio y fin del tamanio del paquete

                // Paso 1: Destuffing y cálculo del CRC. Si todo va bien, obtengo la trama
                // con valores actualizados y sin bytes de CRC.
                tam=destuff_and_check_checksum((unsigned char *)pui8Frame,tam);
                if (tam>=0)
                {
                    //El paquete está bien, luego procedo a tratarlo.
                    ui8Command=decode_command_type(pui8Frame); // Obtencion del byte de Comando
                    tam=get_command_param_pointer(pui8Frame,tam,&ptrtoparam);
                    switch(ui8Command) // Segun el comando tengo que hacer cosas distintas
                    {
                    /** A PARTIR AQUI ES DONDE SE DEBEN AÑADIR NUEVAS RESPUESTAS ANTE LOS COMANDOS QUE SE ENVIEN DESDE LA TIVA **/
                    case COMMAND_PING:  // Algunos comandos no tiene parametros
                        // Crea una ventana popup con el mensaje indicado
                        emit pingReceivedFromTiva();
                        break;

                    case COMMAND_ESTATUS_PERIFERICOS:
                    {
                        PARAMETERS_ESTATUS_PERIFERICOS parametro;
                        if (check_and_extract_command_param(ptrtoparam, tam, sizeof(parametro),&parametro)>0)
                        {
                            emit estatusRecivedFromTiva(parametro.value);
                        }
                    }
                        break;
                    case COMMAND_REJECTED:
                    {
                        // En otros comandos hay que extraer los parametros de la trama y copiarlos
                        // a una estructura para poder procesar su informacion
                        PARAMETERS_COMMAND_REJECTED parametro;
                        if (check_and_extract_command_param(ptrtoparam, tam, sizeof(parametro),&parametro)>0)
                        {
                            // Muestra en una etiqueta (statuslabel) del GUI el mensaje
                            emit commandRejectedFromTiva(parametro.command);
                        }
                        else
                        {
                            emit commandRejectedFromTiva(-1);
                        }
                    }
                        break;
                        //Especificacion 2. Recepción de las muestras

                    case COMMAND_ADC_SAMPLE:
                    {    //SEMANA2: Este caso trata la recepcion de datos del ADC desde la TIVA
                        PARAMETERS_ADC_SAMPLE parametro;
                        if (check_and_extract_command_param(ptrtoparam, tam, sizeof(parametro),&parametro)>0)
                        {
                            // Muestra en una etiqueta (statuslabel) del GUI el mensaje
                            emit ADCSampleReceived(parametro.chan1,parametro.chan2,parametro.chan3,parametro.chan4);
                        }
                        else
                        {   //Si el tamanho de los datos no es correcto emito la senhal statusChanged(...) para reportar un error
                            LastError=QString("Status: Recibidos datos incorrectos");
                            emit statusChanged(QTivaRPC::ReceivedDataError,LastError);
                        }
                    }
                        break;

                    case COMMAND_ADC_SAMPLE8_s:
                    {    //SEMANA2: Este caso trata la recepcion de datos del ADC desde la TIVA
                        PARAMETERS_ADC_SAMPLE8_s parametro;
                        if (check_and_extract_command_param(ptrtoparam, tam, sizeof(parametro),&parametro)>0)
                        {
                            // Muestra en una etiqueta (statuslabel) del GUI el mensaje
                            emit ADCSampleReceived8_s(parametro.chan1,parametro.chan2,parametro.chan3,parametro.chan4);
                        }
                        else
                        {   //Si el tamanho de los datos no es correcto emito la senhal statusChanged(...) para reportar un error
                            LastError=QString("Status: Recibidos datos incorrectos");
                            emit statusChanged(QTivaRPC::ReceivedDataError,LastError);
                        }
                    }
                        break;

                    case COMMAND_ADC_SAMPLE8:
                    {    //SEMANA2: Este caso trata la recepcion de datos del ADC desde la TIVA
                        PARAMETERS_ADC_SAMPLE8 parametro;
                        if (check_and_extract_command_param(ptrtoparam, tam, sizeof(parametro),&parametro)>0)
                        {
                            // Muestra en una etiqueta (statuslabel) del GUI el mensaje
                            uint8_t Aux1;
                            uint8_t Aux2;
                            uint8_t Aux3;
                            uint8_t Aux4;

                            for (int i =0; i<8; i++)
                            {
                                 Aux1 = parametro.chan1[i];
                                 Aux2 = parametro.chan2[i];
                                 Aux3 = parametro.chan3[i];
                                 Aux4 = parametro.chan4[i];
                                 emit ADCSampleReceived8_s(Aux1,Aux2,Aux3,Aux4);
                            }
                        }
                        else
                        {   //Si el tamanho de los datos no es correcto emito la senhal statusChanged(...) para reportar un error
                            LastError=QString("Status: Recibidos datos incorrectos");
                            emit statusChanged(QTivaRPC::ReceivedDataError,LastError);
                        }
                    }
                        break;

                    case COMMAND_ADC_SAMPLE12:
                    {    //SEMANA2: Este caso trata la recepcion de datos del ADC desde la TIVA
                        PARAMETERS_ADC_SAMPLE12 parametro;
                        if (check_and_extract_command_param(ptrtoparam, tam, sizeof(parametro),&parametro)>0)
                        {
                            uint16_t Aux1;
                            uint16_t Aux2;
                            uint16_t Aux3;
                            uint16_t Aux4;

                            for (int i =0; i<4; i++){
                                 Aux1 = parametro.chan1[i];
                                 Aux2 = parametro.chan2[i];
                                 Aux3 = parametro.chan3[i];
                                 Aux4 = parametro.chan4[i];
                                 emit ADCSampleReceived(Aux1,Aux2,Aux3,Aux4);
                            }
                        }
                        else
                        {   //Si el tamanho de los datos no es correcto emito la senhal statusChanged(...) para reportar un error
                            LastError=QString("Status: Recibidos datos incorrectos");
                            emit statusChanged(QTivaRPC::ReceivedDataError,LastError);
                        }
                    }
                        break;

                    case COMMAND_SENSORRGB:
                    {    //SEMANA2: Este caso trata la recepcion de datos del ADC desde la TIVA
                        PARAMETERS_SENSORRGB parametro;
                        if (check_and_extract_command_param(ptrtoparam, tam, sizeof(parametro),&parametro)>0)
                        {
                            // Muestra en una etiqueta (statuslabel) del GUI el mensaje
                            emit SENSOR_RGB_E(parametro.R,parametro.G,parametro.B,parametro.I);
                        }

                        else
                        {   //Si el tamanho de los datos no es correcto emito la senhal statusChanged(...) para reportar un error
                            LastError=QString("Status: Recibidos datos incorrectos");
                            emit statusChanged(QTivaRPC::ReceivedDataError,LastError);
                        }
                    }
                        break;

                    case COMMAND_SENSORPROX_SEND:
                    {    //SEMANA2: Este caso trata la recepcion de datos del ADC desde la TIVA
                        PARAMETERS_SENSORPROX_SEND parametro;
                        if (check_and_extract_command_param(ptrtoparam, tam, sizeof(parametro),&parametro)>0)
                        {
                            // Muestra en una etiqueta (statuslabel) del GUI el mensaje
                            emit SENSORPROX_SEND(parametro.ON,parametro.proximity_data);
                        }

                        else
                        {   //Si el tamanho de los datos no es correcto emito la senhal statusChanged(...) para reportar un error
                            LastError=QString("Status: Recibidos datos incorrectos");
                            emit statusChanged(QTivaRPC::ReceivedDataError,LastError);
                        }
                    }
                        break;

                    case COMMAND_SENSORGEST_SEND:
                    {    //SEMANA2: Este caso trata la recepcion de datos del ADC desde la TIVA
                        PARAMETERS_SENSORGEST_SEND parametro;
                        if (check_and_extract_command_param(ptrtoparam, tam, sizeof(parametro),&parametro)>0)
                        {
                            // Muestra en una etiqueta (statuslabel) del GUI el mensaje
                            emit SENSORGEST_SEND(parametro.GEST);
                        }

                        else
                        {   //Si el tamanho de los datos no es correcto emito la senhal statusChanged(...) para reportar un error
                            LastError=QString("Status: Recibidos datos incorrectos");
                            emit statusChanged(QTivaRPC::ReceivedDataError,LastError);
                        }
                    }
                        break;

                    case COMMAND_SENSOR_SEND:
                    {    //SEMANA2: Este caso trata la recepcion de datos del ADC desde la TIVA
                        PARAMETERS_SENSOR_SEND parametro;
                        if (check_and_extract_command_param(ptrtoparam, tam, sizeof(parametro),&parametro)>0)
                        {
                            // Muestra en una etiqueta (statuslabel) del GUI el mensaje
                            emit SENSOR_SEND(parametro.ON,parametro.proximity_data,parametro.GEST);
                        }

                        else
                        {   //Si el tamanho de los datos no es correcto emito la senhal statusChanged(...) para reportar un error
                            LastError=QString("Status: Recibidos datos incorrectos");
                            emit statusChanged(QTivaRPC::ReceivedDataError,LastError);
                        }
                    }
                        break;

                    case COMMAND_SENSORGEST_SEND_QT:
                    {    //SEMANA2: Este caso trata la recepcion de datos del ADC desde la TIVA
                        PARAMETERS_SENSORGEST_SEND_QT parametro;
                        if (check_and_extract_command_param(ptrtoparam, tam, sizeof(parametro),&parametro)>0)
                        {
                            // Muestra en una etiqueta (statuslabel) del GUI el mensaje
                            uint16_t Aux1;
                            uint16_t Aux2;
                            uint16_t Aux3;
                            uint16_t Aux4;

                            for (int i =0; i<8; i++)
                            {
                                 Aux1 = parametro.u_data[i];
                                 Aux2 = parametro.d_data[i];
                                 Aux3 = parametro.l_data[i];
                                 Aux4 = parametro.r_data[i];
                                 emit SENSORGEST_SEND_QT(Aux1, Aux2, Aux3, Aux4);
                            }
                        }

                        else
                        {   //Si el tamanho de los datos no es correcto emito la senhal statusChanged(...) para reportar un error
                            LastError=QString("Status: Recibidos datos incorrectos");
                            emit statusChanged(QTivaRPC::ReceivedDataError,LastError);
                        }
                    }
                        break;

                    case COMMAND_SENSOR_SEND_QT:
                    {    //SEMANA2: Este caso trata la recepcion de datos del ADC desde la TIVA
                        PARAMETERS_SENSOR_SEND_QT parametro;
                        if (check_and_extract_command_param(ptrtoparam, tam, sizeof(parametro),&parametro)>0)
                        {
                            // Muestra en una etiqueta (statuslabel) del GUI el mensaje
                            uint16_t Aux1;
                            uint16_t Aux2;
                            uint16_t Aux3;
                            uint16_t Aux4;

                            for (int i =0; i<8; i++)
                            {
                                 Aux1 = parametro.u_data[i];
                                 Aux2 = parametro.d_data[i];
                                 Aux3 = parametro.l_data[i];
                                 Aux4 = parametro.r_data[i];
                                 emit SENSOR_SEND_QT(parametro.ON, parametro.proximity_data,Aux1, Aux2, Aux3, Aux4);
                            }
                        }

                        else
                        {   //Si el tamanho de los datos no es correcto emito la senhal statusChanged(...) para reportar un error
                            LastError=QString("Status: Recibidos datos incorrectos");
                            emit statusChanged(QTivaRPC::ReceivedDataError,LastError);
                        }
                    }
                        break;

                    default:
                        //Este error lo notifico mediante la señal statusChanged
                        LastError=QString("Status: Recibido paquete inesperado");
                        emit statusChanged(QTivaRPC::UnexpectedPacketError,LastError);
                        break;
                    }
                }
                else
                {
                    LastError=QString("Status: Error de stuffing o CRC");
                    emit statusChanged(QTivaRPC::CRCorStuffError,LastError);
                    //Avisar con algo, no????
                }
            }
            else
            {

                // B. La trama no está completa o no tiene el tamano adecuado... no lo procesa
                //Este error lo notifico mediante la señal statusChanged
                LastError=QString("Status: Error trozo paquete recibido");
                emit statusChanged(QTivaRPC::FragmentedPacketError,LastError);
            }
            incommingDataBuffer.remove(0,StopCharPosition-StartCharPosition+1); //Elimino el trozo que ya he procesado
        }

        StopCharPosition=incommingDataBuffer.indexOf((char)STOP_FRAME_CHAR,0); //Compruebo si el se ha recibido alguna trama completa mas. (Para ver si tengo que salir del bucle o no
    } //Fin del while....
}

// Este método realiza el establecimiento de la comunicación USB serie con la TIVA a través del interfaz seleccionado
// Se establece una comunicacion a 9600bps 8N1 y sin control de flujo en el objeto
// 'serial' que es el que gestiona la comunicación USB serie en el interfaz QT
// Si la conexion no es correcta, se generan señales de error.
void QTivaRPC::startRPCClient(QString puerto)
{
    if (serial.portName() != puerto) {
        serial.close();
        serial.setPortName(puerto);

        if (!serial.open(QIODevice::ReadWrite)) {
            LastError=QString("No puedo abrir el puerto %1, error code %2")
                         .arg(serial.portName()).arg(serial.error());

            emit statusChanged(QTivaRPC::OpenPortError,LastError);
            return ;
        }

        if (!serial.setBaudRate(9600)) {
            LastError=QString("No puedo establecer tasa de 9600bps en el puerto %1, error code %2")
                         .arg(serial.portName()).arg(serial.error());

            emit statusChanged(QTivaRPC::BaudRateError,LastError);
            return;
        }

        if (!serial.setDataBits(QSerialPort::Data8)) {
            LastError=QString("No puedo establecer 8bits de datos en el puerto %1, error code %2")
                         .arg(serial.portName()).arg(serial.error());


             emit statusChanged(QTivaRPC::DataBitError,LastError);
            return;
        }

        if (!serial.setParity(QSerialPort::NoParity)) {
            LastError=QString("NO puedo establecer parida en el puerto %1, error code %2")
                         .arg(serial.portName()).arg(serial.error());

            emit statusChanged(QTivaRPC::ParityError,LastError);
            return ;
        }

        if (!serial.setStopBits(QSerialPort::OneStop)) {
            LastError=QString("No puedo establecer 1bitStop en el puerto %1, error code %2")
                         .arg(serial.portName()).arg(serial.error());
            emit statusChanged(QTivaRPC::StopError,LastError);
            return;
        }

        if (!serial.setFlowControl(QSerialPort::NoFlowControl)) {
            LastError=QString("No puedo establecer el control de flujo en el puerto %1, error code %2")
                         .arg(serial.portName()).arg(serial.error());
             emit statusChanged(QTivaRPC::FlowControlError,LastError);
            return;
        }
    }

     emit statusChanged(QTivaRPC::TivaConnected,QString(""));

    // Variable indicadora de conexión a TRUE, para que se permita enviar comandos en respuesta
    // a eventos del interfaz gráfico
    connected=true;
}

//Método para leer el último mensaje de error
QString QTivaRPC::getLastErrorMessage()
{
    return LastError;
}

// **** Slots asociados al envio de comandos hacia la TIVA. La estructura va a ser muy parecida en casi todos los
// casos. Se va a crear una trama de un tamaño maximo (100), y se le van a introducir los elementos de
// num_secuencia, comando, y parametros.

//Este Slot realiza el envío de un mensaje de PING a la TIVA
void QTivaRPC::ping()
{
    char paquete[MAX_FRAME_SIZE];
    int size;

    if (connected) // Para que no se intenten enviar datos si la conexion USB no esta activa
    {
        // El comando PING no necesita parametros; de ahí el NULL, y el 0 final.
        // No vamos a usar el mecanismo de numeracion de tramas; pasamos un 0 como n de trama
        size=create_frame((unsigned char *)paquete, COMMAND_PING, NULL, 0, MAX_FRAME_SIZE);
        // Si la trama se creó correctamente, se escribe el paquete por el puerto serie USB
        if (size>0) serial.write(paquete,size);
    }
}


//Este Slot realiza el envío del comando LED a la TIVA
void QTivaRPC::LEDGpio(bool red, bool green, bool blue)
{
    PARAMETERS_LED_GPIO parametro;
    uint8_t pui8Frame[MAX_FRAME_SIZE];
    int size;
    if(connected)
    {
        // Se rellenan los parametros del paquete (en este caso, el estado de los LED)
        parametro.leds.red=red;
        parametro.leds.green=green;
        parametro.leds.blue=blue;
        // Se crea la trama con n de secuencia 0; comando COMANDO_LEDS; se le pasa la
        // estructura de parametros, indicando su tamaño; el nº final es el tamaño maximo
        // de trama
        size=create_frame((uint8_t *)pui8Frame, COMMAND_LED_GPIO, &parametro, sizeof(parametro), MAX_FRAME_SIZE);
        // Se se pudo crear correctamente, se envia la trama
        if (size>0) serial.write((char *)pui8Frame,size);
    }
}


//Este Slot realiza el envio del comando de brillo a la TIVA
void QTivaRPC::LEDPwmBrightness(double value)
{
    PARAMETERS_LED_PWM_BRIGHTNESS parametro;
    uint8_t pui8Frame[MAX_FRAME_SIZE];
    int size;
    if(connected)
    {
        // Se rellenan los parametros del paquete (en este caso, el brillo)
        parametro.rIntensity=(float)value;
        // Se crea la trama con n de secuencia 0; comando COMANDO_LEDS; se le pasa la
        // estructura de parametros, indicando su tamaño; el nº final es el tamaño maximo
        // de trama
        size=create_frame((uint8_t *)pui8Frame, COMMAND_LED_PWM_BRIGHTNESS, &parametro, sizeof(parametro), MAX_FRAME_SIZE);
        // Se se pudo crear correctamente, se envia la trama
        if (size>0) serial.write((char *)pui8Frame,size);
    }
}

//Este Slot realiza el envio del comando de brillo a la TIVA
void QTivaRPC::MODO(bool Cambio)
{
    PARAMETERS_MODO parametro;
    uint8_t pui8Frame[MAX_FRAME_SIZE];
    int size;
    if(connected)
    {
        // Se rellenan los parametros del paquete (en este caso, el brillo)
        parametro.Mode_Cambio = Cambio;
        // Se crea la trama con n de secuencia 0; comando COMANDO_LEDS; se le pasa la
        // estructura de parametros, indicando su tamaño; el nº final es el tamaño maximo
        // de trama
        size=create_frame((uint8_t *)pui8Frame, COMMAND_MODO, &parametro, sizeof(parametro), MAX_FRAME_SIZE);
        // Se se pudo crear correctamente, se envia la trama
        if (size>0) serial.write((char *)pui8Frame,size);
    }
}

//Este Slot realiza el envio del comando de brillo a la TIVA
void QTivaRPC::RGB_MODE(int Rojo_rgb, int Verde_rgb, int Azul_rgb)
{
    PARAMETERS_RGB parametro;
    uint8_t pui8Frame[MAX_FRAME_SIZE];
    int size;
    if(connected)
    {
        // Se rellenan los parametros del paquete (en este caso, el brillo)
        parametro.rojo  = Rojo_rgb;
        parametro.verde = Verde_rgb;
        parametro.azul  = Azul_rgb;
        // Se crea la trama con n de secuencia 0; comando COMANDO_LEDS; se le pasa la
        // estructura de parametros, indicando su tamaño; el nº final es el tamaño maximo
        // de trama
        size=create_frame((uint8_t *)pui8Frame, COMMAND_RGB, &parametro, sizeof(parametro), MAX_FRAME_SIZE);
        // Se se pudo crear correctamente, se envia la trama
        if (size>0) serial.write((char *)pui8Frame,size);
    }
}


void QTivaRPC::ESTATUS_PERIFERICOS()
{
    char paquete[MAX_FRAME_SIZE];
    int size;

    if (connected) // Para que no se intenten enviar datos si la conexion USB no esta activa
    {
        size=create_frame((unsigned char *)paquete, COMMAND_ESTATUS_PERIFERICOS, NULL, 0, MAX_FRAME_SIZE);
        if (size>0) serial.write(paquete,size);
    }
}

//Este Slot realiza el envío del comando LED a la TIVA
void QTivaRPC::INTERRUPCION(bool Interrupcion)
{
    PARAMETERS_INTERRUPCION parametro;
    uint8_t pui8Frame[MAX_FRAME_SIZE];
    int size;
    if(connected)
    {
        parametro.Interrupcion = Interrupcion;
        size=create_frame((uint8_t *)pui8Frame, COMMAND_INTERRUPCION, &parametro, sizeof(parametro), MAX_FRAME_SIZE);

        if (size>0) serial.write((char *)pui8Frame,size);
    }
}

//ESPECIFICACION2: Este Slot permite ordenar al objeto TIVA que envie un comando de conversion
void QTivaRPC::ADCSample(void)
{
    char paquete[MAX_FRAME_SIZE];
    int size;

    if (connected) // Para que no se intenten enviar datos si la conexion USB no esta activa
    {
        size=create_frame((unsigned char *)paquete, COMMAND_ADC_SAMPLE, NULL, 0, MAX_FRAME_SIZE);
        if (size>0) serial.write(paquete,size);
    }
}

void QTivaRPC::ADSGraf(bool Interrupcion, double frecuencia, bool NumMuestras)
{
    PARAMETERS_ADC_GRAF parametro;
    uint8_t pui8Frame[MAX_FRAME_SIZE];
    int size;
    if(connected)
    {
        // Se rellenan los parametros del paquete (en este caso, el estado de los LED)
        parametro.Interrupcion = Interrupcion;
        parametro.NumMuestras = NumMuestras;
        parametro.frecuencia = frecuencia;

        // Se crea la trama con n de secuencia 0; comando COMANDO_LEDS; se le pasa la
        // estructura de parametros, indicando su tamaño; el nº final es el tamaño maximo
        // de trama
        size=create_frame((uint8_t *)pui8Frame, COMMAND_ADC_GRAF, &parametro, sizeof(parametro), MAX_FRAME_SIZE);
        // Se se pudo crear correctamente, se envia la trama
        if (size>0) serial.write((char *)pui8Frame,size);
    }
}

void QTivaRPC::SENSOR_RGB(void)
{
    char paquete[MAX_FRAME_SIZE];
    int size;

    if (connected) // Para que no se intenten enviar datos si la conexion USB no esta activa
    {
        size=create_frame((unsigned char *)paquete, COMMAND_SENSORRGB, NULL, 0, MAX_FRAME_SIZE);
        if (size>0) serial.write(paquete,size);
    }
}

void QTivaRPC::SENSOR_PROX(bool Interrupcion_on, uint8_t Distancia)
{
    PARAMETERS_SENSORPROX parametro;
    uint8_t pui8Frame[MAX_FRAME_SIZE];
    int size;
    if(connected)
    {
        // Se rellenan los parametros del paquete (en este caso, el estado de los LED)
        parametro.Interrupcion_on = Interrupcion_on;
        parametro.Distancia = Distancia;

        size=create_frame((uint8_t *)pui8Frame, COMMAND_SENSORPROX, &parametro, sizeof(parametro), MAX_FRAME_SIZE);
        // Se se pudo crear correctamente, se envia la trama
        if (size>0) serial.write((char *)pui8Frame,size);
    }
}

void QTivaRPC::SENSOR_GEST(bool Interrupcion_on, uint8_t Gest_Qt)
{
    PARAMETERS_SENSORGEST parametro;
    uint8_t pui8Frame[MAX_FRAME_SIZE];
    int size;
    if(connected)
    {
        // Se rellenan los parametros del paquete (en este caso, el estado de los LED)
        parametro.Interrupcion_on = Interrupcion_on;
        parametro.Data_Qt = Gest_Qt;

        size=create_frame((uint8_t *)pui8Frame, COMMAND_SENSORGEST, &parametro, sizeof(parametro), MAX_FRAME_SIZE);
        // Se se pudo crear correctamente, se envia la trama
        if (size>0) serial.write((char *)pui8Frame,size);
    }
}
