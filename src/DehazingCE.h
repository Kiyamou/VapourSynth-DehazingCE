

#ifndef DEHAZINGCE_H_
#define DEHAZINGCE_H_

#include "vapoursynth/VapourSynth.h"
#include "vapoursynth/VSHelper.h"


class dehazing
{
public:
    dehazing(int nW, int nH, int nTBlockSize, float fTransInit, bool bPrevFlag, bool bPosFlag, float fL1, float fL2, int nGBlockSize);
    ~dehazing();

    void RemoveHaze(const uint8_t* src, const uint8_t* refpB, const uint8_t* refpG, const uint8_t* refpR, uint8_t* dst, int stride, int ref_width, int ref_height);

    void AirlightEstimation(const uint8_t* src, int width, int height, int stride);

	float NFTrsEstimationColor(const uint8_t* pnImageR, const uint8_t* pnImageG, const uint8_t* pnImageB, int nStartX, int nStartY, int ref_width, int ref_height);
    void TransmissionEstimationColor(const uint8_t* pnImageR, const uint8_t* pnImageG, const uint8_t* pnImageB, int ref_width, int ref_height);

    void PostProcessing(const uint8_t* src, uint8_t* dst, int width, int height, int stride);  // Called by RestoreImage();
    void RestoreImage(const uint8_t* src, uint8_t* dst, int height, int width, int stride);

    void CalcAcoeff(float* pfSigma, float* pfCov, float* pfA1, float* pfA2, float* pfA3, int nIdx);
    void BoxFilter(float* pfInArray, int nR, int nWid, int nHei, float*& fOutArray);
    void BoxFilter(float* pfInArray1, float* pfInArray2, float* pfInArray3, int nR, int nWid, int nHei, float*& pfOutArray1, float*& pfOutArray2, float*& pfOutArray3);
    void GuidedFilter(int nW, int nH, float fEps);

    void MakeExpLUT();
    void GuideLUTMaker();
    void GammaLUTMaker(float fParameter);

private:
    int width;
    int height;

    int TBlockSize;
    float TransInit;
    float fParameter;

    int GBlockSize;
    int StepSize;
    float GSigma;

    int m_anAirlight[3];
    int m_nAirlight;

    // Airlight search range
    int m_nTopLeftX;
    int m_nTopLeftY;
    int m_nBottomRightX;
    int m_nBottomRightY;

    bool m_PreviousFlag;
    bool m_PostFlag;  // Flag for post processing(deblocking)

    float Lambda1;
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

    float ExpLUT[256];
    float m_pucGammaLUT[256];
    float* m_pfGuidedLUT;
};


#endif
