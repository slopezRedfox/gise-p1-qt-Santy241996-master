#ifndef MAINUSERG
#define MAINUSERGUI_H

#include <QWidget>
#include <QtSerialPort/qserialport.h>
#include <QMessageBox>
#include "qtivarpc.h"

#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>

namespace Ui {
class MainUserGUI;
}

class MainUserGUI : public QWidget
{
    Q_OBJECT

public:
    explicit MainUserGUI(QWidget *parent = 0);
    ~MainUserGUI();

private slots:
    //Slots asociados por nombre
    void on_runButton_clicked();
    void on_serialPortComboBox_currentIndexChanged(const QString &arg1);
    void on_pushButton_clicked();

    void tivaStatusChanged(int status,QString message);
    void pingResponseReceived(void);
    void CommandRejected(int16_t code);

    //Otros slots
    void cambiaLEDs();
    void Cambio_de_Modo();
    void Cambio_Intensidad();
    void Interrupcion_on1();
    void Mod_Led_Status(uint8_t status);
    void on_colorWheel_colorChanged(const QColor &arg1);

    //Segunda parte (ADC)
    void Cambio_ADC_Int();
    void ADCSampleShow(uint16_t chan1,uint16_t chan2,uint16_t chan3,uint16_t chan4);
    void ADCSampleShow8_s(uint8_t chan1,uint8_t chan2,uint8_t chan3,uint8_t chan4);

    //Tercera parte
    void SENSOR_RGB_QT(uint16_t R,uint16_t G,uint16_t B,uint16_t I);
    void SENSOR_PROX_QT(uint8_t ON,uint8_t Proximidad_data);
    void SENSOR_GEST_QT(uint8_t GEST);

    void SENSOR_QT(uint8_t ON,uint8_t Proximidad_data, uint8_t GEST);
    void SENSOR_PROX_RESUME();
    void SENSOR_GEST_RESUME();

    //Cuarta parte
    void GraficaGestos_2(uint8_t u_data, uint8_t d_data,uint8_t l_data, uint8_t r_data);
    void GraficaGestos_3(uint8_t ON,uint8_t Proximidad_data,uint8_t u_data, uint8_t d_data,uint8_t l_data, uint8_t r_data);


private:
    // funciones privadas
    void processError(const QString &s);
    void activateRunButton();

private:
    //Componentes privados
    Ui::MainUserGUI *ui;
    int transactionCount;

    //SEMANA2: Para las graficas
    double xVal[1024]; //valores eje X
    double yVal[4][1024]; //valores ejes Y
    QwtPlotCurve *Channels[4]; //Curvas
    QwtPlotGrid  *m_Grid; //Cuadricula

    //SEMANA4: Para las graficas
    double xVal2[128];          //valores eje X
    double yVal2[128];          //valores ejes Y
    QwtPlotCurve *Channels2;    //Curvas
    QwtPlotGrid  *m_Grid2;      //Cuadricula

    double xVal3[128];          //valores eje X
    double yVal3[128];          //valores ejes Y
    QwtPlotCurve *Channels3;    //Curvas
    QwtPlotGrid  *m_Grid3;      //Cuadricula

    double xVal4[128];          //valores eje X
    double yVal4[128];          //valores ejes Y
    QwtPlotCurve *Channels4;    //Curvas
    QwtPlotGrid  *m_Grid4;      //Cuadricula

    double xVal5[128];          //valores eje X
    double yVal5[128];          //valores ejes Y
    QwtPlotCurve *Channels5;    //Curvas
    QwtPlotGrid  *m_Grid5;      //Cuadricula

    QMessageBox ventanaPopUp;
    QTivaRPC tiva; //Objeto para gestionar la ejecucion acciones en el microcontrolador y/o recibir eventos desde él
};

#endif // GUIPANEL_H
