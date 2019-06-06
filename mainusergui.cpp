#include "mainusergui.h"
#include "ui_mainusergui.h"
#include <QSerialPort>      // Comunicacion por el puerto serie
#include <QSerialPortInfo>  // Comunicacion por el puerto serie
#include <QMessageBox>      // Se deben incluir cabeceras a los componentes que se vayan a crear en la clase
// y que no estén incluidos en el interfaz gráfico. En este caso, la ventana de PopUp <QMessageBox>
// que se muestra al recibir un PING de respuesta

#include<stdint.h>      // Cabecera para usar tipos de enteros con tamaño
#include<stdbool.h>     // Cabecera para usar booleanos

int Cuenta_Grafica;
int Cuenta_Grafica2;

MainUserGUI::MainUserGUI(QWidget *parent) : // Constructor de la clase
    QWidget(parent),
    ui(new Ui::MainUserGUI)                 // Indica que guipanel.ui es el interfaz grafico de la clase
  , transactionCount(0)
{
    ui->setupUi(this);                      // Conecta la clase con su interfaz gráfico.
    setWindowTitle(tr("Interfaz de Control TIVA")); // Título de la ventana

    // Inicializa la lista de puertos serie
    ui->serialPortComboBox->clear(); // Vacía de componentes la comboBox
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        // La descripción realiza un FILTRADO que  nos permite que SOLO aparezcan los interfaces tipo USB serial con el identificador de fabricante
        // y producto de la TIVA
        // NOTA!!: SI QUIERES REUTILIZAR ESTE CODIGO PARA OTRO CONVERSOR USB-Serie, cambia los valores VENDOR y PRODUCT por los correspondientes
        // o cambia la condicion por "if (true) para listar todos los puertos serie"
        if ((info.vendorIdentifier()==0x1CBE) && (info.productIdentifier()==0x0002))
        {
            ui->serialPortComboBox->addItem(info.portName());
        }
    }

    //Inicializa algunos controles
    ui->serialPortComboBox->setFocus();   // Componente del GUI seleccionado de inicio
    ui->pingButton->setEnabled(false);    // Se deshabilita el botón de ping del interfaz gráfico, hasta que

    //Inicializa la ventana de PopUp que se muestra cuando llega la respuesta al PING
    ventanaPopUp.setIcon(QMessageBox::Information);
    ventanaPopUp.setText(tr("Status: RESPUESTA A PING RECIBIDA")); //Este es el texto que muestra la ventana
    ventanaPopUp.setStandardButtons(QMessageBox::Ok);
    ventanaPopUp.setWindowTitle(tr("Evento"));
    ventanaPopUp.setParent(this,Qt::Popup);

    //Conexion de signals de los widgets del interfaz con slots propios de este objeto
    connect(ui->rojo,SIGNAL(toggled(bool)),this,SLOT(cambiaLEDs()));
    connect(ui->verde,SIGNAL(toggled(bool)),this,SLOT(cambiaLEDs()));
    connect(ui->azul,SIGNAL(toggled(bool)),this,SLOT(cambiaLEDs()));

    connect(ui->Knob,SIGNAL(valueChanged(double)),this,SLOT(Cambio_Intensidad()));
    connect(ui->colorWheel,SIGNAL(valueChanged(QColor)),this,SLOT(on_colorWheel_colorChanged(QColor)));
    connect(ui->PWM_MODE,SIGNAL(toggled(bool)),this,SLOT(Cambio_de_Modo()));

    connect(ui->Actualiza ,SIGNAL(clicked(bool)),&tiva,SLOT(ESTATUS_PERIFERICOS()));

    //Conectamos Slots del objeto "Tiva" con Slots de nuestra aplicacion (o con widgets)
    connect(&tiva,SIGNAL(statusChanged(int,QString)),this,SLOT(tivaStatusChanged(int,QString)));
    connect(ui->pingButton,SIGNAL(clicked(bool)),&tiva,SLOT(ping()));
    connect(ui->Interrupcion_on,SIGNAL(toggled(bool)),this,SLOT(Interrupcion_on1()));

    connect(&tiva,SIGNAL(pingReceivedFromTiva()),this,SLOT(pingResponseReceived()));
    connect(&tiva,SIGNAL(commandRejectedFromTiva(int16_t)),this,SLOT(CommandRejected(int16_t)));
    connect(&tiva,SIGNAL(estatusRecivedFromTiva(uint8_t)),this,SLOT(Mod_Led_Status(uint8_t)));

    //ADC. ESPECIFICACION 2.
    connect(&tiva,SIGNAL(ADCSampleReceived(uint16_t,uint16_t,uint16_t,uint16_t)),this,SLOT(ADCSampleShow(uint16_t,uint16_t,uint16_t,uint16_t)));
    connect(&tiva,SIGNAL(ADCSampleReceived8_s(uint8_t,uint8_t,uint8_t,uint8_t)),this,SLOT(ADCSampleShow8_s(uint8_t,uint8_t,uint8_t,uint8_t)));

    connect(&tiva,SIGNAL(ADCSampleReceived8(uint64_t,uint64_t,uint64_t,uint64_t)),this,SLOT(ADCSampleShow8(uint64_t,uint64_t,uint64_t,uint64_t)));
    connect(&tiva,SIGNAL(ADCSampleReceived12(uint64_t,uint64_t,uint64_t,uint64_t)),this,SLOT(ADCSampleShow12(uint64_t,uint64_t,uint64_t,uint64_t)));

    connect(ui->ADCButton,SIGNAL(clicked(bool)),&tiva,SLOT(ADCSample()));

    connect(ui->frecuencia,SIGNAL(sliderReleased()),this,SLOT(Cambio_ADC_Int()));
    connect(ui->NumMuestras,SIGNAL(clicked(bool)),this,SLOT(Cambio_ADC_Int()));
    connect(ui->AUTOADC,SIGNAL(toggled(bool)),this,SLOT(Cambio_ADC_Int()));

    //SENSOR. ESPECIFICACION 3.
    connect(ui->BottonRGBsen,SIGNAL(clicked(bool)),&tiva,SLOT(SENSOR_RGB()));
    connect(&tiva,SIGNAL(SENSOR_RGB_E(uint16_t,uint16_t,uint16_t,uint16_t)),this,SLOT(SENSOR_RGB_QT(uint16_t,uint16_t,uint16_t,uint16_t)));

    connect(ui->Int_Prox_On,SIGNAL(clicked(bool)),this,SLOT(SENSOR_PROX_RESUME()));
    connect(ui->Int_Prox_Con,SIGNAL(sliderReleased()),this,SLOT(SENSOR_PROX_RESUME()));
    connect(&tiva,SIGNAL(SENSORPROX_SEND(uint8_t,uint8_t)),this,SLOT(SENSOR_PROX_QT(uint8_t,uint8_t)));

    connect(ui->Int_Gest_On,SIGNAL(clicked(bool)),this,SLOT(SENSOR_GEST_RESUME()));
    connect(&tiva,SIGNAL(SENSORGEST_SEND(uint8_t)),this,SLOT(SENSOR_GEST_QT(uint8_t)));
    connect(&tiva,SIGNAL(SENSOR_SEND(uint8_t,uint8_t,uint8_t)),this,SLOT(SENSOR_QT(uint8_t,uint8_t,uint8_t)));

    //SENSOR. ESPECIFICACION 4.
    connect(ui->Int_Gest_Qt,SIGNAL(clicked(bool)),this,SLOT(SENSOR_GEST_RESUME()));
    connect(&tiva,SIGNAL(SENSORGEST_SEND_QT(uint8_t,uint8_t,uint8_t,uint8_t)),this,SLOT(GraficaGestos_2(uint8_t,uint8_t,uint8_t,uint8_t)));
    connect(&tiva,SIGNAL(SENSOR_SEND_QT(uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t)),this,SLOT(GraficaGestos_3(uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t)));

    //-----------------------------------------------------------------------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------------------------------------------------------------------
    //Inicializacion GRAFICA
    ui->Grafica->setTitle("Sinusoides");                    // Titulo de la grafica
    ui->Grafica->setAxisTitle(QwtPlot::xBottom, "Tiempo");  // Etiqueta eje X de coordenadas
    ui->Grafica->setAxisTitle(QwtPlot::yLeft, "Valor");     // Etiqueta eje Y de coordenadas
    ui->Grafica->axisAutoScale(true);                       // Con Autoescala
    ui->Grafica->setAxisScale(QwtPlot::yLeft, 0, 3.3);      // Escala fija
    ui->Grafica->setAxisScale(QwtPlot::xBottom,0,1024.0);

    // Formateo de la curva
    for(int i=0;i<4;i++){
    Channels[i] = new QwtPlotCurve();
    Channels[i]->attach(ui->Grafica);
    }

    //Inicializacion de los valores básicos
    for(int i=0;i<1024;i++){
            xVal[i]=i;
            yVal[0][i]=0;
            yVal[1][i]=0;
            yVal[2][i]=0;
            yVal[3][i]=0;
    }

    Channels[0]->setRawSamples(xVal,yVal[0],1024);
    Channels[1]->setRawSamples(xVal,yVal[1],1024);
    Channels[2]->setRawSamples(xVal,yVal[2],1024);
    Channels[3]->setRawSamples(xVal,yVal[3],1024);

    Channels[0]->setPen(QPen(Qt::red)); // Color de la curva
    Channels[1]->setPen(QPen(Qt::cyan));
    Channels[2]->setPen(QPen(Qt::yellow));
    Channels[3]->setPen(QPen(Qt::green));

    m_Grid = new QwtPlotGrid();     // Rejilla de puntos
    m_Grid->attach(ui->Grafica);    // que se asocia al objeto QwtPl
    ui->Grafica->setAutoReplot(false); //Desactiva el autoreplot (mejora la eficiencia)
    //FIN Inicializacion GRAFICA
    //-----------------------------------------------------------------------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------------------------------------------------------------------
    //Inicializacion GRAFICA
    ui->Grafica_2->setTitle("Up");                    // Titulo de la grafica
    ui->Grafica_2->setAxisTitle(QwtPlot::xBottom, "Tiempo");  // Etiqueta eje X de coordenadas
    ui->Grafica_2->setAxisTitle(QwtPlot::yLeft, "Valor");     // Etiqueta eje Y de coordenadas
    ui->Grafica_2->axisAutoScale(true);                       // Con Autoescala
    ui->Grafica_2->setAxisScale(QwtPlot::yLeft, 0, 255);      // Escala fija
    ui->Grafica_2->setAxisScale(QwtPlot::xBottom,0,128.0);

    Channels2 = new QwtPlotCurve();
    Channels2 ->attach(ui->Grafica_2);

    //Inicializacion de los valores básicos
    for(int i=0;i<128;i++){
            xVal2[i]=i;
            yVal2[i]=0;
    }

    Channels2->setRawSamples(xVal2,yVal2,128);
    Channels2->setPen(QPen(Qt::red)); // Color de la curva

    m_Grid2 = new QwtPlotGrid();     // Rejilla de puntos
    m_Grid2->attach(ui->Grafica_2);    // que se asocia al objeto QwtPl
    ui->Grafica_2->setAutoReplot(false); //Desactiva el autoreplot (mejora la eficiencia)

    //-----------------------------------------------------------------------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------------------------------------------------------------------
    //Inicializacion GRAFICA
    ui->Grafica_3->setTitle("Down");                    // Titulo de la grafica
    ui->Grafica_3->setAxisTitle(QwtPlot::xBottom, "Tiempo");  // Etiqueta eje X de coordenadas
    ui->Grafica_3->setAxisTitle(QwtPlot::yLeft, "Valor");     // Etiqueta eje Y de coordenadas
    ui->Grafica_3->axisAutoScale(true);                       // Con Autoescala
    ui->Grafica_3->setAxisScale(QwtPlot::yLeft, 0, 255);      // Escala fija
    ui->Grafica_3->setAxisScale(QwtPlot::xBottom,0,128.0);

    Channels3 = new QwtPlotCurve();
    Channels3 ->attach(ui->Grafica_3);

    //Inicializacion de los valores básicos
    for(int i=0;i<128;i++){
            xVal3[i]=i;
            yVal3[i]=0;
    }

    Channels3->setRawSamples(xVal3,yVal3,128);
    Channels3->setPen(QPen(Qt::green)); // Color de la curva

    m_Grid3 = new QwtPlotGrid();     // Rejilla de puntos
    m_Grid3->attach(ui->Grafica_3);    // que se asocia al objeto QwtPl
    ui->Grafica_3->setAutoReplot(false); //Desactiva el autoreplot (mejora la eficiencia)

    //-----------------------------------------------------------------------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------------------------------------------------------------------
    //Inicializacion GRAFICA
    ui->Grafica_4->setTitle("Left");                    // Titulo de la grafica
    ui->Grafica_4->setAxisTitle(QwtPlot::xBottom, "Tiempo");  // Etiqueta eje X de coordenadas
    ui->Grafica_4->setAxisTitle(QwtPlot::yLeft, "Valor");     // Etiqueta eje Y de coordenadas
    ui->Grafica_4->axisAutoScale(true);                       // Con Autoescala
    ui->Grafica_4->setAxisScale(QwtPlot::yLeft, 0, 255);      // Escala fija
    ui->Grafica_4->setAxisScale(QwtPlot::xBottom,0,128.0);

    Channels4 = new QwtPlotCurve();
    Channels4 ->attach(ui->Grafica_4);

    //Inicializacion de los valores básicos
    for(int i=0;i<128;i++){
            xVal4[i]=i;
            yVal4[i]=0;
    }

    Channels4->setRawSamples(xVal4,yVal4,128);
    Channels4->setPen(QPen(Qt::cyan)); // Color de la curva

    m_Grid4 = new QwtPlotGrid();     // Rejilla de puntos
    m_Grid4->attach(ui->Grafica_4);    // que se asocia al objeto QwtPl
    ui->Grafica_4->setAutoReplot(false); //Desactiva el autoreplot (mejora la eficiencia)

    //-----------------------------------------------------------------------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------------------------------------------------------------------
    //Inicializacion GRAFICA
    ui->Grafica_5->setTitle("Right");                    // Titulo de la grafica
    ui->Grafica_5->setAxisTitle(QwtPlot::xBottom, "Tiempo");  // Etiqueta eje X de coordenadas
    ui->Grafica_5->setAxisTitle(QwtPlot::yLeft, "Valor");     // Etiqueta eje Y de coordenadas
    ui->Grafica_5->axisAutoScale(true);                       // Con Autoescala
    ui->Grafica_5->setAxisScale(QwtPlot::yLeft, 0, 255);      // Escala fija
    ui->Grafica_5->setAxisScale(QwtPlot::xBottom,0,128.0);

    Channels5 = new QwtPlotCurve();
    Channels5 ->attach(ui->Grafica_5);

    //Inicializacion de los valores básicos
    for(int i=0;i<128;i++){
            xVal5[i]=i;
            yVal5[i]=0;
    }

    Channels5->setRawSamples(xVal5,yVal5,128);
    Channels5->setPen(QPen(Qt::yellow)); // Color de la curva

    m_Grid5 = new QwtPlotGrid();     // Rejilla de puntos
    m_Grid5->attach(ui->Grafica_5);    // que se asocia al objeto QwtPl
    ui->Grafica_5->setAutoReplot(false); //Desactiva el autoreplot (mejora la eficiencia)

    //--------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------
}

MainUserGUI::~MainUserGUI() // Destructor de la clase
{
    delete ui;   // Borra el interfaz gráfico asociado a la clase
}

void MainUserGUI::on_serialPortComboBox_currentIndexChanged(const QString &arg1)
{
    activateRunButton();
}

// Funcion auxiliar de procesamiento de errores de comunicación
void MainUserGUI::processError(const QString &s)
{
    activateRunButton(); // Activa el botón RUN
    // Muestra en la etiqueta de estado la razón del error (notese la forma de pasar argumentos a la cadena de texto)
    ui->statusLabel->setText(tr("Status: Not running, %1.").arg(s));
}

// Funcion de habilitacion del boton de inicio/conexion
void MainUserGUI::activateRunButton()
{
    ui->runButton->setEnabled(true);
}

//Este Slot lo hemos conectado con la señal statusChange de la TIVA, que se activa cuando
//El puerto se conecta o se desconecta y cuando se producen determinados errores.
//Esta función lo que hace es procesar dichos eventos
void MainUserGUI::tivaStatusChanged(int status,QString message)
{
    switch (status)
    {
        case QTivaRPC::TivaConnected:

            //Caso conectado..
            // Deshabilito el boton de conectar
            ui->runButton->setEnabled(false);

            // Se indica que se ha realizado la conexión en la etiqueta 'statusLabel'
            ui->statusLabel->setText(tr("Ejecucion, conectado al puerto %1.")
                             .arg(ui->serialPortComboBox->currentText()));

            //    // Y se habilitan los controles deshabilitados
            ui->pingButton->setEnabled(true);

        break;

        case QTivaRPC::TivaDisconnected:
            //Caso desconectado..
            // Rehabilito el boton de conectar
            ui->runButton->setEnabled(false);
        break;
        case QTivaRPC::UnexpectedPacketError:
        case QTivaRPC::FragmentedPacketError:
        case QTivaRPC::CRCorStuffError:
            //Errores detectados en la recepcion de paquetes
            ui->statusLabel->setText(message);
        break;
        default:
            //Otros errores (por ejemplo, abriendo el puerto)
            processError(tiva.getLastErrorMessage());
    }
}


// SLOT asociada a pulsación del botón RUN
void MainUserGUI::on_runButton_clicked()
{
    //Intenta arrancar la comunicacion con la TIVA
    tiva.startRPCClient( ui->serialPortComboBox->currentText());
}

//_________________________________________________________________________________________
//_________________________________________________________________________________________

void MainUserGUI::cambiaLEDs(void)
{
    //Invoca al metodo LEDGPio para enviar la orden a la TIVA
    tiva.LEDGpio(ui->rojo->isChecked(),ui->verde->isChecked(),ui->azul->isChecked());
}

void MainUserGUI::Cambio_de_Modo(void)
{
    //Invoca al metodo LEDGPio para enviar la orden a la TIVA
    tiva.MODO(ui->PWM_MODE->isChecked());
}

void MainUserGUI::Cambio_Intensidad(void)
{
    //Invoca al metodo LEDGPio para enviar la orden a la TIVA
    tiva.LEDPwmBrightness(ui->Knob->value());
}


void MainUserGUI::on_colorWheel_colorChanged(const QColor &arg1)
{
//    //Poner aqui el codigo para pedirle al objeto "tiva" (clase QRemoteTIVA) que envíe la orden de cambiar el Color hacia el microcontrolador

    tiva.RGB_MODE(arg1.red(),arg1.green(),arg1.blue());
    ui->Azul_Ver->display(arg1.blue());
    ui->Rojo_Ver->display(arg1.red());
    ui->Verde_Ver->display(arg1.green());
}

void MainUserGUI::Interrupcion_on1(void)
{
    //Invoca al metodo LEDGPio para enviar la orden a la TIVA
    tiva.INTERRUPCION(ui->Interrupcion_on->isChecked());
}


void MainUserGUI::Cambio_ADC_Int(void)
{
    //Invoca al metodo LEDGPio para enviar la orden a la TIVA
    tiva.ADSGraf(ui->AUTOADC->isChecked(),ui->frecuencia->value(),ui->NumMuestras->isChecked());
}

void MainUserGUI::SENSOR_PROX_RESUME(void)
{
    //Invoca al metodo LEDGPio para enviar la orden a la TIVA
    tiva.SENSOR_PROX(ui->Int_Prox_On->isChecked(),ui->Int_Prox_Con->value());
}

void MainUserGUI::SENSOR_GEST_RESUME(void)
{
    //Invoca al metodo LEDGPio para enviar la orden a la TIVA
    tiva.SENSOR_GEST(ui->Int_Gest_On->isChecked(),ui->Int_Gest_Qt->isChecked());
}

//_________________________________________________________________________________________
//_________________________________________________________________________________________

//Slots Asociado al boton que limpia los mensajes del interfaz
void MainUserGUI::on_pushButton_clicked()
{
    ui->statusLabel->setText(tr(""));
}

//**** Slots asociados a la recepción de mensajes desde la TIVA ********/
//Están conectados a señales generadas por el objeto TIVA de clase QTivaRPC (se conectan en el constructor de la ventana, más arriba en este fichero))
//Se pueden añadir los que se quieran para completar la aplicación

//Este se ejecuta cuando se recibe una respuesta de PING
void MainUserGUI::pingResponseReceived()
{
    // Muestra una ventana popUP para el caso de comando PING; no te deja definirla en un "caso"
    ventanaPopUp.setStyleSheet("background-color: lightgrey");
    ventanaPopUp.setModal(true); //CAMBIO: Se sustituye la llamada a exec(...) por estas dos.
    ventanaPopUp.show();
}


//Este se ejecuta cuando se recibe un mensaje de comando rechazado
void MainUserGUI::CommandRejected(int16_t code)
{
    ui->statusLabel->setText(tr("Status: Comando rechazado,%1").arg(code));
}

void MainUserGUI::Mod_Led_Status(uint8_t status)
{
    if(status & 0x01){ui->led_P1->setChecked(1);}
    else{ui->led_P1->setChecked(0);}

    if(status & 0x20){ui->led_P2->setChecked(1);}
    else{ui->led_P2->setChecked(0);}

    if(status & 0x02){ui->led_Lr->setChecked(1);}
    else{ui->led_Lr->setChecked(0);}

    if(status & 0x08){ui->led_Lg->setChecked(1);}
    else{ui->led_Lg->setChecked(0);}

    if(status & 0x04){ui->led_Lb->setChecked(1);}
    else{ui->led_Lb->setChecked(0);}
}

//ESPECIFICACION 2. Mostrar los valores ADC
void MainUserGUI::ADCSampleShow(uint16_t chan1,uint16_t chan2,uint16_t chan3,uint16_t chan4)
{

    //Manda cada dato a su correspondiente display (pasandolos a voltios)
    ui->lcdCh1->display(((double)chan1)*3.3/4096.0);
    ui->lcdCh2->display(((double)chan2)*3.3/4096.0);
    ui->lcdCh3->display(((double)chan3)*3.3/4096.0);
    ui->lcdCh4->display(((double)chan4)*3.3/4096.0);


    if (Cuenta_Grafica == 1024)
    {
        Cuenta_Grafica = 0;
        for(int i=0;i<1024;i++){
                xVal[i]=i;
                yVal[0][i]=0;
                yVal[1][i]=0;
                yVal[2][i]=0;
                yVal[3][i]=0;
        }
    }
    else
    {
        Cuenta_Grafica = Cuenta_Grafica + 1;
    }

    yVal[0][Cuenta_Grafica]= ((double)chan1)*3.3/4096.0;
    yVal[1][Cuenta_Grafica]= ((double)chan2)*3.3/4096.0;
    yVal[2][Cuenta_Grafica]= ((double)chan3)*3.3/4096.0;
    yVal[3][Cuenta_Grafica]= ((double)chan4)*3.3/4096.0;

    ui->Grafica->replot(); //Refresca la grafica una vez actualizados los valores
}

void MainUserGUI::ADCSampleShow8_s(uint8_t chan1,uint8_t chan2,uint8_t chan3,uint8_t chan4)
{
    uint16_t chan11 = chan1 <<4;
    uint16_t chan22 = chan2 <<4;
    uint16_t chan33 = chan3 <<4;
    uint16_t chan44 = chan4 <<4;


    //Manda cada dato a su correspondiente display (pasandolos a voltios)
    ui->lcdCh1->display(((double)chan11)*3.3/4096.0);
    ui->lcdCh2->display(((double)chan22)*3.3/4096.0);
    ui->lcdCh3->display(((double)chan33)*3.3/4096.0);
    ui->lcdCh4->display(((double)chan44)*3.3/4096.0);


    if (Cuenta_Grafica == 1024)
    {
        Cuenta_Grafica = 0;
        for(int i=0;i<1024;i++){
                xVal[i]=i;
                yVal[0][i]=0;
                yVal[1][i]=0;
                yVal[2][i]=0;
                yVal[3][i]=0;
        }
    }
    else
    {
        Cuenta_Grafica = Cuenta_Grafica + 1;
    }

    yVal[0][Cuenta_Grafica]= ((double)chan11)*3.3/4096.0;
    yVal[1][Cuenta_Grafica]= ((double)chan22)*3.3/4096.0;
    yVal[2][Cuenta_Grafica]= ((double)chan33)*3.3/4096.0;
    yVal[3][Cuenta_Grafica]= ((double)chan44)*3.3/4096.0;

    ui->Grafica->replot(); //Refresca la grafica una vez actualizados los valores
}

//ESPECIFICACION 3.

//Mostrar los valores del SENSOR en modo RGB
void MainUserGUI::SENSOR_RGB_QT(uint16_t R, uint16_t G,uint16_t B,uint16_t I)
{
    QColor arg1;
    double Rm;
    double Gm;
    double Bm;

    Rm = ((double)R);
    Gm = ((double)G);
    Bm = ((double)B);
    float Aux;

    if ((Rm > 255)||(Gm > 255)||(Bm > 255))
    {
        if((Rm>=Bm) && (Rm>=Gm))
        {
            Aux = 255/Rm;
            Rm  = Rm*Aux;
            Gm  = Gm*Aux;
            Bm  = Bm*Aux;
        }
        if((Gm>=Rm) && (Gm>=Bm))
        {
            Aux = 255/Gm;
            Rm  = Rm*Aux;
            Gm  = Gm*Aux;
            Bm  = Bm*Aux;
        }
        if((Bm>=Gm) && (Bm>=Rm))
        {
            Aux = 255/Bm;
            Rm  = Rm*Aux;
            Gm  = Gm*Aux;
            Bm  = Bm*Aux;
        }
    }

    //Manda cada dato a su correspondiente display (pasandolos a voltios)
    ui->lcdChR_RGB->display(Rm);
    ui->lcdChG_RGB->display(Gm);
    ui->lcdChB_RGB->display(Bm);
    ui->lcdChI_RGB->display((double)I);

    arg1.setRed((double)R);
    arg1.setGreen((double)G);
    arg1.setBlue((double)B);

    ui->ColorSelector->setColor(arg1);
}

void MainUserGUI::SENSOR_PROX_QT(uint8_t ON, uint8_t Proximidad_data)
{
    ui->lcdCh_Prox->display(((double)Proximidad_data));
    ui->led_AlarmaProx->setChecked(ON);
}

void MainUserGUI::SENSOR_GEST_QT(uint8_t GEST)
{
    ui->led_UP   ->setChecked(GEST & 0x01);
    ui->led_DOWN ->setChecked(GEST & 0x02);
    ui->led_RIGHT->setChecked(GEST & 0x04);
    ui->led_LEFT ->setChecked(GEST & 0x08);
    ui->led_NEAR ->setChecked(GEST & 0x10);
    ui->led_FAR  ->setChecked(GEST & 0x20);
}

void MainUserGUI::SENSOR_QT(uint8_t ON, uint8_t Proximidad_data,uint8_t GEST)
{
    ui->led_UP   ->setChecked(GEST & 0x01);
    ui->led_DOWN ->setChecked(GEST & 0x02);
    ui->led_RIGHT->setChecked(GEST & 0x04);
    ui->led_LEFT ->setChecked(GEST & 0x08);
    ui->led_NEAR ->setChecked(GEST & 0x10);
    ui->led_FAR  ->setChecked(GEST & 0x20);

    ui->lcdCh_Prox->display(((double)Proximidad_data));
    ui->led_AlarmaProx->setChecked(ON);
}


//Parte 4

void MainUserGUI::GraficaGestos_2(uint8_t u_data,uint8_t d_data,uint8_t l_data,uint8_t r_data)
{
    //Manda cada dato a su correspondiente display (pasandolos a voltios)
    if (Cuenta_Grafica2 == 128)
    {
        Cuenta_Grafica2 = 0;
        for(int i=0;i<128;i++){
                xVal2[i]=i;
                yVal2[i]=0;
                yVal3[i]=0;
                yVal4[i]=0;
                yVal5[i]=0;
        }
    }
    else
    {
        Cuenta_Grafica2 = Cuenta_Grafica2 + 1;
    }

    yVal2[Cuenta_Grafica2]= ((double)u_data);
    yVal3[Cuenta_Grafica2]= ((double)d_data);
    yVal4[Cuenta_Grafica2]= ((double)l_data);
    yVal5[Cuenta_Grafica2]= ((double)r_data);

    ui->Grafica_2->replot(); //Refresca la grafica una vez actualizados los valores
    ui->Grafica_3->replot(); //Refresca la grafica una vez actualizados los valores
    ui->Grafica_4->replot(); //Refresca la grafica una vez actualizados los valores
    ui->Grafica_5->replot(); //Refresca la grafica una vez actualizados los valores
}

void MainUserGUI::GraficaGestos_3(uint8_t ON, uint8_t Proximidad_data, uint8_t u_data,uint8_t d_data,uint8_t l_data,uint8_t r_data)
{
    ui->lcdCh_Prox->display(((double)Proximidad_data));
    ui->led_AlarmaProx->setChecked(ON);

    //Manda cada dato a su correspondiente display (pasandolos a voltios)
    if (Cuenta_Grafica2 == 128)
    {
        Cuenta_Grafica2 = 0;
        for(int i=0;i<128;i++){
                xVal2[i]=i;
                yVal2[i]=0;
                yVal3[i]=0;
                yVal4[i]=0;
                yVal5[i]=0;
        }
    }
    else
    {
        Cuenta_Grafica2 = Cuenta_Grafica2 + 1;
    }
    yVal2[Cuenta_Grafica2]= ((double)u_data);
    yVal3[Cuenta_Grafica2]= ((double)d_data);
    yVal4[Cuenta_Grafica2]= ((double)l_data);
    yVal5[Cuenta_Grafica2]= ((double)r_data);

    ui->Grafica_2->replot(); //Refresca la grafica una vez actualizados los valores
    ui->Grafica_3->replot(); //Refresca la grafica una vez actualizados los valores
    ui->Grafica_4->replot(); //Refresca la grafica una vez actualizados los valores
    ui->Grafica_5->replot(); //Refresca la grafica una vez actualizados los valores
}
