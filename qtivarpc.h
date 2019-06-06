#ifndef QTIVARPC_H
#define QTIVARPC_H

#include <QObject>
#include <QSerialPort>      // Comunicacion por el puerto serie
#include <QSerialPortInfo>  // Comunicacion por el puerto serie


#include<stdint.h>      // Cabecera para usar tipos de enteros con tamaño
#include<stdbool.h>     // Cabecera para usar booleanos




class QTivaRPC : public QObject
{

    Q_OBJECT
public:
    explicit QTivaRPC(QObject *parent = 0);

    //Define una serie de etiqueta para los errores y estados notificados por la señal statusChanged(...)
    enum TivaStatus {TivaConnected,
                     TivaDisconnected,
                     OpenPortError,
                     BaudRateError,
                     DataBitError,
                     ParityError,
                     StopError,
                     FlowControlError,
                     UnexpectedPacketError,
                     FragmentedPacketError,
                     CRCorStuffError,
                     ReceivedDataError,
                    };
    Q_ENUM(TivaStatus)

    //Metodo publico
    QString getLastErrorMessage();

signals:
    void statusChanged(int status, QString message); //Esta señal se genera al realizar la conexión/desconexion o cuando se produce un error de comunicacion
    void pingReceivedFromTiva(void); //Esta señal se genera al recibir una respuesta de PING de la TIVA
    void commandRejectedFromTiva(int16_t code); //Esta señal se genera al recibir una respuesta de Comando Rechazado desde la TIVA
    void estatusRecivedFromTiva(uint8_t status);
    void ADCSampleReceived(uint16_t chan1,uint16_t chan2,uint16_t chan3,uint16_t chan4);
    void ADCSampleReceived8_s(uint8_t chan1,uint8_t chan2,uint8_t chan3,uint8_t chan4);
    void SENSOR_RGB_E(uint16_t R,uint16_t G,uint16_t B,uint16_t I);
    void SENSORPROX_SEND(uint8_t ON,uint8_t Proximidad_data);
    void SENSORGEST_SEND(uint8_t GEST);
    void SENSOR_SEND(uint8_t ON,uint8_t Proximidad_data,uint8_t GEST);
    void SENSORGEST_SEND_QT(uint8_t u_data, uint8_t d_data,uint8_t l_data, uint8_t r_data);
    void SENSOR_SEND_QT(uint8_t ON,uint8_t Proximidad_data,uint8_t u_data, uint8_t d_data,uint8_t l_data, uint8_t r_data);

public slots:
    void startRPCClient(QString puerto); //Este Slot arranca la comunicacion
    void ping(void); //Este Slot provoca el envio del comando PING
    void LEDGpio(bool red, bool green, bool blue); //Este Slot provoca el envio del comando LED
    void LEDPwmBrightness(double value); //Este Slot provoca el envio del comando Brightness
    void MODO(bool Cambio);
    void RGB_MODE(int Rojo_rgb, int Verde_rgb, int Azul_rgb);
    void ESTATUS_PERIFERICOS (void);
    void INTERRUPCION(bool Interrupcion);
    void ADCSample(void);
    void ADSGraf(bool automatica, double frecuencia, bool NumMuestra);
    void SENSOR_RGB (void);
    void SENSOR_PROX (bool Interrupcion_on,uint8_t Distancia);
    void SENSOR_GEST (bool Interrupcion_on, uint8_t Gest_Qt);

private slots:
    void processIncommingSerialData(); //Este Slot se conecta a la señal readyRead(..) del puerto serie. Se encarga de procesar y decodificar los mensajes que llegan de la TIVA y
                        //generar señales para algunos de ellos.

private:
    QSerialPort serial;
    QString LastError;
    bool connected;
    QByteArray incommingDataBuffer;

};

#endif // QTIVARPC_H

