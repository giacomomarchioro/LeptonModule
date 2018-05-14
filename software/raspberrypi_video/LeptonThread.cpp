#include "LeptonThread.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include "Palettes.h"
#include "SPI.h"
#include "Lepton_I2C.h"

#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QString>

#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <QApplication>

#define PACKET_SIZE 164
#define PACKET_SIZE_UINT16 (PACKET_SIZE/2)
#define PACKETS_PER_FRAME 60
#define FRAME_SIZE_UINT16 (PACKET_SIZE_UINT16*PACKETS_PER_FRAME)
#define FPS 27;
static int raw [120][160];
int x = 0;

LeptonThread::LeptonThread() : QThread(){
}

LeptonThread::~LeptonThread(){
}

void LeptonThread::run(){
	//create the initial image
	myImage = QImage(80, 60, QImage::Format_RGB888);

	//open spi port
	SpiOpenPort(0);

    //Define the RAW file, where 1st value is T_sensor in K, next RAW!
    x++;
    // Raw file format: binary, one word for min value, one for max value, then 80*60 words of raw data
    QDateTime time_now = QDateTime::currentDateTime();
    QString tempo = time_now.toString("yyyyMMdd_hhmmsszzz.bin");
    QStringList path1 = QApplication::arguments();
    QString fileN = path1[1];
    fileN.append("/");
    fileN.append(tempo);
    std::cout <<fileN.toStdString();
    QFile rawFile(fileN);
    rawFile.open(QIODevice::Truncate | QIODevice::ReadWrite);
    QDataStream rawOut(&rawFile);

	while(true) {

		//read data packets from lepton over SPI
		int resets = 0;
		for(int j=0;j<PACKETS_PER_FRAME;j++) {
			//if it's a drop packet, reset j to 0, set to -1 so he'll be at 0 again loop
			read(spi_cs0_fd, result+sizeof(uint8_t)*PACKET_SIZE*j, sizeof(uint8_t)*PACKET_SIZE);
			int packetNumber = result[j*PACKET_SIZE+1];
			if(packetNumber != j) {
				j = -1;
				resets += 1;
				usleep(1000);
				//Note: we've selected 750 resets as an arbitrary limit, since there should never be 750 "null" packets between two valid transmissions at the current poll rate
				//By polling faster, developers may easily exceed this count, and the down period between frames may then be flagged as a loss of sync
				if(resets == 750) {
					SpiClosePort(0);
					usleep(750000);
					SpiOpenPort(0);
				}
			}
		}
		if(resets >= 30) {
			qDebug() << "done reading, resets: " << resets;
		}

        frameBuffer = (uint16_t *)result;
        double t = camera_tmpKelvin();
        double t_aux = aux_tmpKelvin();

        QDateTime time_now = QDateTime::currentDateTime();
        //QString tempo = time_now.toString("hhmmsszzz");
        QString tempo = time_now.toString("hhmm");
        QString temposecms = time_now.toString("sszzz");
        double tempo2 = tempo.toDouble();
        double tempo3 = temposecms.toDouble();


        //Sctritture dei primi quattro dati nel file RAW...
        rawOut << (uint16_t)tempo2 << (uint16_t)tempo3 <<(uint16_t)t << (uint16_t) t_aux;

        int row, column;
        uint16_t value;int i = 0;
		uint16_t minValue = 65535;
		uint16_t maxValue = 0;
		
		for(int i=0;i<FRAME_SIZE_UINT16;i++) {
			//skip the first 2 uint16_t's of every packet, they're 4 header bytes
			if(i % PACKET_SIZE_UINT16 < 2) {
				continue;
			}
			
			//flip the MSB and LSB at the last second
			int temp = result[i*2];
			result[i*2] = result[i*2+1];
			result[i*2+1] = temp;
            rawOut << frameBuffer[i];
			value = frameBuffer[i];

			if(value > maxValue) {
				maxValue = value;
			}
			if(value < minValue) {
				minValue = value;
			}
			column = i % PACKET_SIZE_UINT16 - 2;
			row = i / PACKET_SIZE_UINT16 ;
		}


		float diff = maxValue - minValue;
		float scale = 255/diff;
        float valueCenter = 0;
        QRgb color;

		for(int i=0;i<FRAME_SIZE_UINT16;i++) {
			if(i % PACKET_SIZE_UINT16 < 2) {
				continue;
			}
			value = (frameBuffer[i] - minValue) * scale;
            const int *colormap = colormap_ironblack;
            //const int *colormap = colormap_rainbow;
			color = qRgb(colormap[3*value], colormap[3*value+1], colormap[3*value+2]);
			column = (i % PACKET_SIZE_UINT16 ) - 2;
			row = i / PACKET_SIZE_UINT16;

            raw[row][column] = frameBuffer[i];
            if(column == 80 && row == 60)
                valueCenter = frameBuffer[i];

			myImage.setPixel(column, row, color);
		}

		//lets emit the signal for update
		emit updateImage(myImage);

	}
    rawFile.close();
	
	//finally, close SPI port just bcuz
	SpiClosePort(0);
}

void LeptonThread::performFFC() {
	//perform FFC
    lepton_perform_ffc();
}

void LeptonThread::save_pgm_file(void){
    //save image
    char image_name[32];
    int image_index = 0;

    do{
        sprintf(image_name, "IMG_%.4d.png", image_index);
        image_index += 1;
        if (image_index > 9999){
            image_index = 0;
            break;
        }
    } while(access(image_name, F_OK) == 0);
    myImage.save(image_name);
}

// export the file in binati in 14 bit and jpg
void LeptonThread::export_jpg() {
    //JPG image, top quality
    int t = 0;
    myImage.save(QString("rgb%1.jpg").arg(t), "JPG", 100);
}

// get the temperatur of the sensor in Kelvin
void LeptonThread::getTmpKelvin_Sensor(){
    double a = camera_tmpKelvin();
    double b = aux_tmpKelvin();
    printf("FPA temp K: %f \nAUX temp K: %f\n", a/100.0,b/100.0);

}

void LeptonThread::getTmpCelsius_Sensor(){
    //get the temperatur of sensor in Celsius
    camera_tmpCelsius();
    //double t = camera_tmpCelsius();
    //printf("FPA temp Celsius: %.2f\n", t);
}


void LeptonThread::export_Raw2Celsius(){
    double t = camera_tmpKelvin();
    //variables
    struct stat buf;
    const char *inicio = "Celsius_T_";
    const char *fim = ".txt";
    char meio[32];
    //convert from int to string
    sprintf(meio, "%f", t);
    char name[64];

    //---------------------- criate txt -----------------------
    //creating file name
    strcpy(name, inicio);
    strcat(name, meio);
    strcat(name, fim);

    FILE *arq1 = fopen(name,"wt");
    char valors[64];

    for(int i = 0; i < 60; i++){
        for(int j = 0; j < 80; j++){
            sprintf(valors, "%f", raw2Celsius(raw[i][j]));
            fputs(valors, arq1);
            fputs(" ", arq1);
        }
        fputs("\n", arq1);
    }
    fclose(arq1);
}












