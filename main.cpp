// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <QSet>
#include <iostream>
#include <iomanip>
#include "tiffio.h"
#include <time.h>

using namespace std;

bool getImage(string path, uint32*& raster, uint32 &w, uint32 &h, size_t &npixels) {
    TIFF* tif = TIFFOpen(path.c_str(), "r");
    if(tif) {
        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
        npixels = w * h;
        raster = new uint32[npixels];

        if(raster != NULL) {
            if(TIFFReadRGBAImage(tif, w, h, raster, 0)) {
                TIFFClose(tif);
                return true;
            }
        }
    }
    TIFFClose(tif);
    return false;
}

QSet<size_t>* avgBrokenPixelSearch(uint32* raster, uint32 w, size_t npixels, const double error, uint8 k) {
    /* Смещения индекса пикселя для выбора соседних пикселей
     * 0 - текущий пиксель
     * 5 6 7
     * 3   4
     * 0 1 2
     *
     * 19 20 21 22 23
     * 14 15 16 17 18
     * 10 11    12 13
     *  5  6  7  8  9
     *  0  1  2  3  4
     */
    size_t* adjacentPositions;
    uint8 adjSize;
    size_t lineBorder;
    if(k == 3) {
        adjSize = 8;
        adjacentPositions = new size_t[adjSize]{0, 1, 2, w, w+2, 2*w, 2*w+1, 2*w+2};
        lineBorder = w-2;
    }
    else {
        adjSize = 24;
        adjacentPositions = new size_t[adjSize]{0, 1, 2, 3, 4, w, w+1, w+2, w+3, w+4, 2*w, 2*w+1, 2*w+3, 2*w+4, 3*w, 3*w+1, 3*w+2, 3*w+3, 3*w+4, 4*w, 4*w+1, 4*w+2, 4*w+3, 4*w+4};
        lineBorder = w-4;
    }
    const size_t comparedPosition = adjacentPositions[adjSize - 1] / 2;
    uint16 sum[4];
    bool isBrokenPixel;
    QSet<size_t>* brokenPixels = new QSet<size_t>;
    double delta;
    for(size_t i = 0; i < npixels-adjacentPositions[adjSize - 1]; i++) {
        if(!(i%w < lineBorder))
            continue;
        for(uint8 channel = 0; channel < 4; channel++)
            sum[channel] = 0;
        for(uint8 pos = 0; pos < adjSize; pos++) {
            for(uint8 channel = 0; channel < 4; channel++) {
                sum[channel] += (raster[i + adjacentPositions[pos]] >> channel*8) & 0xff;
            }
        }
        isBrokenPixel = false;

        for(uint8 channel = 0; channel < 4; channel++) {
            delta = double(sum[channel])/adjSize - ((raster[i + comparedPosition] >> channel*8) & 0xff);
            if(delta >= 0 && delta > error) {
                isBrokenPixel = true;
                break;
            } else if(delta < 0 && delta < -error) {
                isBrokenPixel = true;
                break;
            }
        }
        if(isBrokenPixel)
            *brokenPixels << i + comparedPosition;
    }
    delete[] adjacentPositions;
    return brokenPixels;
}

//QSet<size_t>* assocRulesBrokenPixelSearch(uint32* raster, uint32 w, size_t npixels, const double error) {
//    // p - позиция проверяемого пикселя, i - текущая позиция в массиве
//    // . . . 5 . . .
//    // . . . 4 . . .
//    // . . . 3 . . .
//    // 0 1 2 p 3 4 5
//    // . . . 2 . . .
//    // . . . 1 . . .
//    // i . . 0 . . .
//    const size_t horizontalPositions[] {w, w+1, w+2, w+4, w+5, w+6};
//    const size_t verticalPositions[] {3, w+3, 2*w+3, 3*w+3, 4*w+3, 5*w+3};


//}

uint8 median(uint8 f, uint8 s, uint8 t) {
    if(f < s){
        if(s < t) return s;
        if(f < t) return t;
        return f;
    }
    if(t < s) return s;
    if(f < t) return f;
    return t;
}

QSet<size_t>* medianBrokenPixelSearch(uint32* raster, uint32 w, size_t npixels, const double error) {
    const size_t poss[4][2] {{w, w+2}, {1, 2*w+1}, {0, 2*w+2}, {2*w, 2}};
    const size_t cPos = w+1; // позиция проверяемого пикселя
    uint8 cPixel; // значение проверяемого пикселя
    uint16 channels[4];
    bool isBrokenPixel;
    QSet<size_t>* brokenPixels = new QSet<size_t>;
    int16 delta;

    for(size_t i = 0; i < npixels-w-w-2; i++) {
        if(!(i%w < w-2))
            continue;
        for(uint8 channel = 0; channel < 4; channel++) {
            cPixel = (raster[i + cPos] >> channel*8) & 0xff;
            channels[channel] = median(
                    cPixel,
                    median(
                        cPixel,
                        median( cPixel, ((raster[i + poss[0][0]] >> channel*8) & 0xff), ((raster[i + poss[0][1]] >> channel*8) & 0xff)),
                        median( cPixel, ((raster[i + poss[1][0]] >> channel*8) & 0xff), ((raster[i + poss[1][1]] >> channel*8) & 0xff))
                    ),
                    median(
                        cPixel,
                        median( cPixel, ((raster[i + poss[2][0]] >> channel*8) & 0xff), ((raster[i + poss[2][1]] >> channel*8) & 0xff)),
                        median( cPixel, ((raster[i + poss[3][0]] >> channel*8) & 0xff), ((raster[i + poss[3][1]] >> channel*8) & 0xff))
                    )
            );
        }

        isBrokenPixel = false;
        for(uint8 channel = 0; channel < 4; channel++) {
            delta = channels[channel] - ((raster[i + cPos] >> channel*8) & 0xff);
            if(delta >= 0 && delta > error) {
                isBrokenPixel = true;
                break;
            } else if(delta < 0 && delta < -error) {
                isBrokenPixel = true;
                break;
            }
        }
        if(isBrokenPixel)
            *brokenPixels << i + cPos;
    }
    return brokenPixels;
}

size_t chosePixel(double* P){
    uint8 maxI = 0;
    for(uint8 i = 1; i < 8; i++){
        if(P[maxI] < P[i]) maxI = i;
    }
    return maxI;
}

QSet<size_t>* hierarchyBrokenPixelSearch(uint32* raster, uint32 w, size_t npixels, const double error) {
    QSet<size_t>* brokenPixels = new QSet<size_t>;
    size_t directions[8] = {0, 1, 2, w, w+2, 2*w, 2*w+1, 2*w+2};
    size_t comparedPixel = w+1;
    uint16** sumsRasterRGBA = new uint16* [npixels]; // суммы значений соседних пикселей по каналам
    uint8* samePixels = new uint8[npixels]; // количество соседних пикселей того же цвета

    for(size_t i = 0; i < npixels; i++) {
        sumsRasterRGBA[i] = new uint16[4] {0,0,0,0};
        if(i < npixels - 2*w-2) {
            samePixels[i] = 0;
            for(uint8 dir = 0, channel; dir < 8; dir++) {
                for(channel = 0; channel < 4; channel++) {
                    sumsRasterRGBA[i][channel] += (raster[i + directions[dir]] >> channel*8) & 0xff;
                }
                if(((raster[i + comparedPixel] >> channel*8) & 0xff) == ((raster[i + directions[dir]] >> channel*8) & 0xff))
                    samePixels[i]++;
            }
        }
    }

    uint16 avgNeighborPixelRGBA[8][4]; // средние значения окружающих пикселей для 8 соседних пикселей
    double sumAvgNPixelsRGBA [4]; // сумма средних значений соседних пикселей
    size_t checkingPos, secondCheckingPos; // позиция обрабатываемого пикселя; позиция потивоположного checkingPos пикселя относительно центрально
    double P[8]; // вероятность выбора пикселя

    double samePixelsSum; // сумма 8 соседних пикселей из массива samePixels
    double V[8]; // вероятность выбора пикселя по критерию 2

    uint16 diffOppositePixelsRGBA[4][4]; // разница противолежащих пикселей
    double sumDiffsRGBA[4]; // сумма разниц
    double W[8]; // вероятность выбора пикселя по критерию 3

    const uint16 M = 255, avgM = M*7, diffM = M*8;

    bool isBrokenPixel;
    double delta;
    for(size_t i = 0; i < npixels - 2*w-2; i++){
        if(!(i%w < w-2))
            continue;
        for(uint8 a = 0; a < 8; a++){
            P[a] = 0;
            V[a] = 0;
            W[a] = 0;
        }
        for(uint8 channel = 0; channel < 4; channel++){
            sumAvgNPixelsRGBA[channel] = avgM;
            sumDiffsRGBA[channel] = diffM;
        }
        samePixelsSum = 0;
        for(uint8 dir = 0, channel; dir < 8; dir++){
            checkingPos = i + directions[dir];
            if(dir < 4)
                secondCheckingPos = i + directions[7 - dir];
            for(channel = 0; channel < 4; channel++) {
                avgNeighborPixelRGBA[dir][channel] = (sumsRasterRGBA[checkingPos][channel] - ((raster[i + comparedPixel] >> channel*8) & 0xff)) / 7; // 7 - pixelToAvg
                sumAvgNPixelsRGBA[channel] -= avgNeighborPixelRGBA[dir][channel];
                if(dir < 4) {
                    if(((raster[checkingPos] >> channel*8) & 0xff) > ((raster[secondCheckingPos] >> channel*8) & 0xff)) {
                        diffOppositePixelsRGBA[dir][channel] = ((raster[checkingPos] >> channel*8) & 0xff) - ((raster[secondCheckingPos] >> channel*8) & 0xff);
                        sumDiffsRGBA[channel] -= diffOppositePixelsRGBA[dir][channel];
                    }
                    else {
                        diffOppositePixelsRGBA[dir][channel] = ((raster[secondCheckingPos] >> channel*8) & 0xff) - ((raster[checkingPos] >> channel*8) & 0xff);
                        sumDiffsRGBA[channel] -= diffOppositePixelsRGBA[dir][channel];
                    }
                }
            }

            if(samePixels[i] != 0){
                V[dir] = samePixels[i + directions[dir]];
                samePixelsSum += V[dir];
            }
        }
        for(uint8 dir = 0, channel; dir < 8; dir++) {
            for(channel = 0; channel < 4; channel++){
                P[dir] += (M - avgNeighborPixelRGBA[dir][channel]) / sumAvgNPixelsRGBA[channel];
            }
            if(V[dir] != 0){
                P[dir] += V[dir] / samePixelsSum;
            }
            if(dir < 4)
                for(channel = 0; channel < 4; channel++){
                    W[dir] = (M - diffOppositePixelsRGBA[dir][channel]) / sumDiffsRGBA[channel];
                    P[dir] += W[dir];
                    P[7 - dir] += W[dir];
                }
        }

        isBrokenPixel = false;
        for(uint8 channel = 0; channel < 4; channel++) {
            delta = int16((raster[i + comparedPixel] >> channel*8) & 0xff) - int16((raster[i + directions[chosePixel(P)]] >> channel*8) & 0xff);
            if(delta >= 0 && delta > error) {
                isBrokenPixel = true;
                break;
            } else if(delta < 0 && delta < -error) {
                isBrokenPixel = true;
                break;
            }
        }
        if(isBrokenPixel) {
            *brokenPixels << i + comparedPixel;
        }
    }

    // delete
    for(size_t i = 0 ; i < npixels; i++) {
        delete[] sumsRasterRGBA[i];
    }
    delete[] sumsRasterRGBA;
    delete[] samePixels;
    return brokenPixels;
}

int main()
{
    time_t start, end;

    uint32* raster = nullptr; uint32 w = 0, h = 0; size_t npixels = 0;
    uint8 numberOfMethods = 4;
    QSet<size_t>** brokenPixels = new QSet<size_t>*[numberOfMethods];
    QSet<size_t> commonBrokenPixels;
    double error = 25;

    if(getImage("D:\\source\\Qt\\untitled\\white_2.tif", raster, w, h, npixels)) {
        start = clock();
        brokenPixels[0] = avgBrokenPixelSearch(raster, w, npixels, error, 3);
        end = clock();
        cout << "avg3 milliseconds: " << end - start << endl;

        start = clock();
        brokenPixels[1] = avgBrokenPixelSearch(raster, w, npixels, error, 5);
        end = clock();
        cout << "avg5 milliseconds: " << end - start << endl;

        start = clock();
        brokenPixels[2] = medianBrokenPixelSearch(raster, w, npixels, error);
        end = clock();
        cout << "median3 milliseconds: " << end - start << endl;

        start = clock();
        brokenPixels[3] = hierarchyBrokenPixelSearch(raster, w, npixels, error);
        end = clock();
        cout << "hierarchy3 milliseconds: " << end - start << endl;
    }

    for(uint8 method = 0; method < numberOfMethods; method++) {
        commonBrokenPixels.unite(*(brokenPixels[method]));
    }
    QList<size_t> outputList(commonBrokenPixels.begin(), commonBrokenPixels.end());
    sort(outputList.begin(), outputList.end());

    cout << "Pixels total: " << outputList.count() << endl;
    cout << setw(11) << setfill(' ') << "";
    for(uint8 method = 0; method < numberOfMethods; method++) {
        cout << setw(9) << setfill(' ') << "Method " + to_string(method);
    }
    cout << endl;
    double counter;
    for(auto el: outputList){
        cout << setw(11) << setfill(' ') << "(" + to_string(el/w) + ";" + to_string(el%w) + ")";
        counter = 0;
        for(uint8 method = 0; method < numberOfMethods; method++) {
            if(brokenPixels[method]->contains(el)){
                counter++;
                cout << setw(9) << setfill(' ') << "True";
            }
            else {
                cout << setw(9) << setfill(' ') << "False";
            }
        }
        cout << "  " << counter/numberOfMethods*100 << "%" << endl;
    }

    delete[] raster;
    for(uint8 i = 0; i < numberOfMethods; i++)
        delete brokenPixels[i];
    delete[] brokenPixels;
}
