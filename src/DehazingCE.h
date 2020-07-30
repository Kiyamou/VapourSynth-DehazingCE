

#ifndef DEHAZINGCE_H_
#define DEHAZINGCE_H_

#include "vapoursynth/VapourSynth.h"
#include "vapoursynth/VSHelper.h"


class dehazing
{
public:
    dehazing(int nW, int nH, int n_refW, int n_refH, int nBits, int nABlockSize, int nTBlockSize, float fTransInit, bool bPrevFlag, bool bPosFlag, double dL1, float fL2, int nGBlockSize);
    ~dehazing();

    template <typename T>
    void RemoveHaze(const T* src, const T* refpB, const T* refpG, const T* refpR, T* dst, int stride);

    void MakeExpLUT();
    void GuideLUTMaker();
    void GammaLUTMaker(float fParameter);

private:
    template <typename T>
    void AirlightEstimation(const T* src, int _width, int _height, int stride);

    template <typename T>
    float NFTrsEstimationColor(const T* pnImageR, const T* pnImageG, const T* pnImageB, int nStartX, int nStartY);

    void UpsampleTransmission();

    template <typename T>
    void TransmissionEstimationColor(const T* pnImageR, const T* pnImageG, const T* pnImageB);

    template <typename T>
    void PostProcessing(const T* src, T* dst, int width, int height, int stride);  // Called by RestoreImage();

    template <typename T>
    void RestoreImage(const T* src, T* dst, int height, int width, int stride);

    void CalcAcoeff(float* pfSigma, float* pfCov, float* pfA1, float* pfA2, float* pfA3, int nIdx);
    void BoxFilter(float* pfInArray, int nR, int nWid, int nHei, float*& fOutArray);
    void BoxFilter(float* pfInArray1, float* pfInArray2, float* pfInArray3, int nR, int nWid, int nHei, float*& pfOutArray1, float*& pfOutArray2, float*& pfOutArray3);
    void GuidedFilter(int nW, int nH, float fEps);

private:
    int width;
    int height;
    int ref_width;
    int ref_height;

    int peak;
    int bits;

    int TBlockSize;
    float TransInit;
    float fParameter;

    int GBlockSize;
    int StepSize;
    float GSigma;

    int ABlockSize;
    int m_anAirlight[3] = { 0 };
    int m_nAirlight;

    // Airlight search range
    int m_nTopLeftX;
    int m_nTopLeftY;
    int m_nBottomRightX;
    int m_nBottomRightY;

    bool m_PreviousFlag;
    bool m_PostFlag;  // Flag for post processing(deblocking)

    double Lambda1;
    float Lambda2;

    int TopLeftX;
    int TopLeftY;
    int BottomRightX;
    int BottomRightY;

    int* m_pnRImg;
    int* m_pnGImg;
    int* m_pnBImg;

    float* m_pfTransmission;   // preliminary transmission
    float* m_pfTransmissionR;  // refined transmission
    float* m_pfSmallTrans;

    float ExpLUT[65536];
    float m_pucGammaLUT[65536];
    float* m_pfGuidedLUT;
};


#endif
