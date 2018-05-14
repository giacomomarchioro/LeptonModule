#include <QApplication>
#include <QThread>
#include <QMutex>
#include <QMessageBox>
#include <QDoubleSpinBox>
#include <QColor>
#include <QLabel>
#include <QtDebug>
#include <QString>
#include <QPushButton>
#include <QImage>
#include <QLCDNumber>

#include "LeptonThread.h"

#include "MyLabel.h"



#include <QDate>
#include <stdio.h>
#include <QTime>
#include <QDateTime>
#include <QString>
#include <iostream>
using namespace std;


int main( int argc, char **argv )
{
    int WindowWidth = 560*2;
    int WindowHeight = 320*2;
    int ImageWidth = 320*2;
    int ImageHeight = 240*2;

	//create the app
	QApplication a( argc, argv );
	
	QWidget *myWidget = new QWidget;
    myWidget->setGeometry(400, 300, WindowWidth, WindowHeight);
    myWidget->setWindowTitle("ThermalEndoscop");
    //myWidget->setBackgroundRole("White");

	//create an image placeholder for myLabel
	//fill the top left corner with red, just bcuz
	QImage myImage;
    //myImage = QImage(320, 240, QImage::Format_RGB888);
    myImage = QImage(ImageWidth, ImageHeight, QImage::Format_RGB888);
	QRgb red = qRgb(255,0,0);
	for(int i=0;i<80;i++) {
		for(int j=0;j<60;j++) {

			myImage.setPixel(i, j, red);
		}
	}

	//create a label, and set it's image to the placeholder
	MyLabel myLabel(myWidget);
    myLabel.setGeometry(185, 10, ImageWidth, ImageHeight);
	myLabel.setPixmap(QPixmap::fromImage(myImage));

    //labet to indicate ambient temperature
    QLabel *tmpAmbLabel = new QLabel("T amb", myWidget);
    tmpAmbLabel->setGeometry(10, 120, 80, 30);
    tmpAmbLabel->setFont(QFont("Times", 12, QFont::Bold));

    QDoubleSpinBox *dsb1 = new QDoubleSpinBox(myWidget);
    dsb1->setGeometry(80, 123, 95, 25);
    dsb1->setValue(35);
    dsb1->setSuffix(" \xBA C");

    //label to indicate reflect temperature
    QLabel *tmpRefLabel = new QLabel("T ref", myWidget);
    tmpRefLabel->setGeometry(10, 160, 80, 30);
    tmpRefLabel->setFont(QFont("Times", 12, QFont::Bold));

    QDoubleSpinBox *dsb2 = new QDoubleSpinBox(myWidget);
    dsb2->setGeometry(80, 163, 95, 25);
    dsb2->setValue(35);
    dsb2->setSuffix(" \xBA C");

    // Label to set the emisivity
    QLabel *emisivita = new QLabel("Emisivita", myWidget);
    emisivita->setGeometry(10, 200, 110, 30);
    emisivita->setFont(QFont("Times", 12, QFont::Bold));

    QDoubleSpinBox *dsb3 = new QDoubleSpinBox(myWidget);
    dsb3->setGeometry(100, 203, 70, 25);
    dsb3->setValue(0.98);
    dsb3->setRange(0.01, 1.00);

    //labet to indicate distance between cam and object
    QLabel *tmpDistLabel = new QLabel("Dist", myWidget);
    tmpDistLabel->setGeometry(10, 240, 110, 30);
    tmpDistLabel->setFont(QFont("Times", 12, QFont::Bold));

    QDoubleSpinBox *dsb4 = new QDoubleSpinBox(myWidget);
    dsb4->setGeometry(80, 243, 95, 25);
    dsb4->setValue(0.15);
    dsb4->setSuffix(" m");



    //label to see the FAC temperatur sensor in kelvin
    QLabel *tmpLabel = new QLabel("T sensor [\xBA C]", myWidget);
    tmpLabel->setGeometry(10, 320, 110, 30);
    tmpLabel->setFont(QFont("Times", 12, QFont::Bold));

    QLCDNumber *lcd = new QLCDNumber(5, myWidget);
    lcd->setSegmentStyle(QLCDNumber::Filled);
    lcd->setGeometry(110, 323, 65, 25);
    lcd->display(34.51);


    //create a FFC button
    QPushButton *button1 = new QPushButton("FFC", myWidget);
    button1->setGeometry(10, WindowHeight-35, 105, 30);
    button1->setFont(QFont("Times", 12, QFont::Bold));

    //create a Colorbar button
    QPushButton *button2 = new QPushButton("FPA Temp[K]", myWidget);
    button2->setGeometry(120, WindowHeight-35, 115, 30);
    button2->setFont(QFont("Times", 12, QFont::Bold));

    //create a file of RAW Date in format .mat
    QPushButton *button3 = new QPushButton("ExportCelsius", myWidget);
    button3->setGeometry(240, WindowHeight-35, 120, 30);
    button3->setFont(QFont("Times", 12, QFont::Bold));

    //export file in format PNG
    QPushButton *button4 = new QPushButton("Save PNG" , myWidget);
    button4->setGeometry(360, WindowHeight-35, 100, 30);
    button4->setFont(QFont("Times", 12, QFont::Bold));

	
	//create a thread to gather SPI data
	//when the thread emits updateImage, the label should update its image accordingly
	LeptonThread *thread = new LeptonThread();
	QObject::connect(thread, SIGNAL(updateImage(QImage)), &myLabel, SLOT(setImage(QImage)));

    //QObject::connect(lcd, SIGNAL(updateLCD(getTmpCelsius_Sensor())), thread, SLOT(getTmpCelsius_Sensor()));
    //thread->start();

	//connect ffc button to the thread's ffc action
	QObject::connect(button1, SIGNAL(clicked()), thread, SLOT(performFFC()));
	thread->start();
	
    //connect colorbar button to the thread's temperatur action
    QObject::connect(button2, SIGNAL(clicked()), thread, SLOT(getTmpKelvin_Sensor()));
	thread->start();

    //button to export file in binary 14 bit
    QObject::connect(button3, SIGNAL(clicked()), thread, SLOT(export_Raw2Celsius()));
    thread->start();

    //connect saveImage button to the thread's make foto
    QObject::connect(button4, SIGNAL(clicked()), thread, SLOT(save_pgm_file()));
    thread->start();


	myWidget->show();

	return a.exec();
}

