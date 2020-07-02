

#include "DehazingCE.h"
#include "Helper.hpp"

constexpr float SQRT_3 = 1.733f;

dehazing::dehazing(int nW, int nH, int nBits, int nABlockSize, int nTBlockSize, float fTransInit, bool bPrevFlag, bool bPosFlag, double dL1, float fL2, int nGBlockSize)
{
    width = nW;
    height = nH;
    peak = (1 << nBits) - 1;
    bits = nBits;

    // Flags for temporal coherence & post processing
    m_PreviousFlag = bPrevFlag;
    m_PostFlag = bPosFlag;

    // Parameters for each cost (loss cost, temporal coherence cost)
    Lambda1 = dL1;
    Lambda2 = fL2;  // only used in previous mode

    // Block size for transmission estimation
    TBlockSize = nTBlockSize;
    TransInit = fTransInit;

    // Guided filter block size, step size(sampling step), & LookUpTable parameter
    GBlockSize = nGBlockSize;
    StepSize = 2;
    GSigma = 10.f;

    // Block size for air estimation
    ABlockSize = nABlockSize;

    // Specify the region of atmospheric light estimation
    TopLeftX = 0;
    TopLeftY = 0;
    BottomRightX = width;
    BottomRightY = height;

    m_pfTransmission  = new float[width * height];
    m_pfTransmissionR = new float[width * height];

    m_pfImageR = new float[width * height];
    m_pfImageG = new float[width * height];
    m_pfImageB = new float[width * height];

    m_pfGuidedLUT = new float[GBlockSize * GBlockSize];
}

dehazing::~dehazing()
{
    if (m_pfTransmission != nullptr)
        delete[] m_pfTransmission;
    if (m_pfTransmissionR != nullptr)
        delete[] m_pfTransmissionR;

    if (m_pfImageR != nullptr)
        delete[] m_pfImageR;
    if (m_pfImageG != nullptr)
        delete[] m_pfImageG;
    if (m_pfImageB != nullptr)
        delete[] m_pfImageB;

    if (m_pfGuidedLUT != nullptr)
        delete[] m_pfGuidedLUT;

    m_pfTransmission = nullptr;
    m_pfTransmissionR = nullptr;

    m_pfImageR = nullptr;
    m_pfImageG = nullptr;
    m_pfImageB = nullptr;

    m_pfGuidedLUT = nullptr;
}


template <typename T>
void dehazing::RemoveHaze(const T* src, const T* refpB, const T* refpG, const T* refpR, T* dst, int stride, int ref_width, int ref_height)
{
    float fEps = 0.001f;

    // Converting to float point
    for (auto j = 0; j < height; j++)
    {
        for (auto i = 0; i < width; i++)
        {
            const auto pos = (j * width + i) * 3;
            m_pfImageR[i] = (float)src[pos];
            m_pfImageG[i] = (float)src[pos + 1];
            m_pfImageB[i] = (float)src[pos + 2];
        }
    }

    AirlightEstimation(src, width, height, stride);
    TransmissionEstimationColor(refpB, refpG, refpR, ref_width, ref_height);
    GuidedFilter(width, height, fEps);
    RestoreImage(src, dst, width, height, stride);
}

/*
    Function: RestoreImage
    Description: Dehazed the image using estimated transmission and atmospheric light.
    Parameter:
        imInput - Input hazy image.
    Return:
        imOutput - Dehazed image.
 */
template <typename T>
void dehazing::RestoreImage(const T* src, T* dst, int width, int height, int stride)
{
    // post processing flag
    if (m_PostFlag == true)
    {
        PostProcessing(src, dst, width, height, stride);
    }
    else
    {
        //#pragma omp parallel for
        for (auto j = 0; j < height; j++)
        {
            for (auto i = 0; i < width; i++)
            {
                // I' = (I - Airlight) / Transmission + Airlight and Gamma correction using Lut
                const auto pos = (j * width + i) * 3;
                const float transmission = clamp(m_pfTransmissionR[j * width + i], 0.f, 1.f); // m_pfTransmissionR calculated in GuideFilter

                dst[pos]     = (T)m_pucGammaLUT[clamp((int)((src[pos]     - m_anAirlight[0]) / transmission + m_anAirlight[0]), 0, peak)];
                dst[pos + 1] = (T)m_pucGammaLUT[clamp((int)((src[pos + 1] - m_anAirlight[1]) / transmission + m_anAirlight[1]), 0, peak)];
                dst[pos + 2] = (T)m_pucGammaLUT[clamp((int)((src[pos + 2] - m_anAirlight[2]) / transmission + m_anAirlight[2]), 0, peak)];
            }
        }
    }
}

/*
    Function: PostProcessing
    Description: deblocking for blocking artifacts of mpeg video sequence.
    Parameter:
        imInput - Input hazy frame.
    Return:
        imOutput - Dehazed frame.
 */
template <typename T>
void dehazing::PostProcessing(const T* src, T* dst, int width, int height, int stride)
{
    const int nNumStep = 10;
    const int nDisPos = 20;

#pragma omp parallel for private(nAD0, nAD1, nAD1, nS)
    for (auto j = 0; j < height; j++)
    {
        for (auto i = 0; i < width; i++)
        {
            // I' = (I - Airlight) / Transmission + Airlight and Gamma correction using Lut
            const auto pos = (j * width + i) * 3;
            const float transmission = clamp(m_pfTransmissionR[j * width + i], 0.f, 1.f);

            dst[pos]     = (T)m_pucGammaLUT[clamp((int)((src[pos]     - m_anAirlight[0]) / transmission + m_anAirlight[0]), 0, peak)];
            dst[pos + 1] = (T)m_pucGammaLUT[clamp((int)((src[pos + 1] - m_anAirlight[1]) / transmission + m_anAirlight[1]), 0, peak)];
            dst[pos + 2] = (T)m_pucGammaLUT[clamp((int)((src[pos + 2] - m_anAirlight[2]) / transmission + m_anAirlight[2]), 0, peak)];

            // If transmission is less than 0.4, apply post processing because more dehazed block yields more artifacts
            if (i > nDisPos + nNumStep && m_pfTransmissionR[j * width + i - nDisPos] < 0.4)
            {
                const auto posD  = (j * width + (i - nDisPos)) * 3;
                const auto posDp = (j * width + (i - nDisPos - 1)) * 3;

                float nAD0 = (float)(dst[posD]     - dst[posDp]);
                float nAD1 = (float)(dst[posD + 1] - dst[posDp + 1]);
                float nAD2 = (float)(dst[posD + 2] - dst[posDp + 2]);

                if (std::max(std::max(abs(nAD0), abs(nAD1)), abs(nAD2)) < 20 &&
                      abs(dst[posDp]     - dst[(j * width + (i - nDisPos - 1 - nNumStep)) * 3])
                    + abs(dst[posDp + 1] - dst[(j * width + (i - nDisPos - 1 - nNumStep)) * 3 + 1])
                    + abs(dst[posDp + 2] - dst[(j * width + (i - nDisPos - 1 - nNumStep)) * 3 + 2])
                    + abs(dst[posD]      - dst[(j * width + (i - nDisPos - 1 - nNumStep)) * 3])
                    + abs(dst[posD + 1]  - dst[(j * width + (i - nDisPos - 1 - nNumStep)) * 3 + 1])
                    + abs(dst[posD + 2]  - dst[(j * width + (i - nDisPos - 1 - nNumStep)) * 3 + 2]) < 30)
                {
                    for (auto nS = 1; nS < nNumStep + 1; nS++)
                    {
                        dst[(j * width + (i - nDisPos - 1 + nS - nNumStep)) * 3]     = (T)clamp((float)dst[(j * width + (i - nDisPos - 1 + nS - nNumStep)) * 3]     + (float)nS * nAD0 / (float)nNumStep, 0.f, (float)peak);
                        dst[(j * width + (i - nDisPos - 1 + nS - nNumStep)) * 3 + 1] = (T)clamp((float)dst[(j * width + (i - nDisPos - 1 + nS - nNumStep)) * 3 + 1] + (float)nS * nAD1 / (float)nNumStep, 0.f, (float)peak);
                        dst[(j * width + (i - nDisPos - 1 + nS - nNumStep)) * 3 + 2] = (T)clamp((float)dst[(j * width + (i - nDisPos - 1 + nS - nNumStep)) * 3 + 2] + (float)nS * nAD2 / (float)nNumStep, 0.f, (float)peak);
                    }
                }
            }
        }
    }
}

template <typename T>
void dehazing::TransmissionEstimationColor(const T* pnImageR, const T* pnImageG, const T* pnImageB, int ref_width, int ref_height)
{
    for (auto y = 0; y < ref_height; y += TBlockSize)
    {
        for (auto x = 0; x < ref_width; x += TBlockSize)
        {
            float fTrans = NFTrsEstimationColor(pnImageR, pnImageG, pnImageB, x, y, ref_width, ref_height);
            for (auto yStep = y; yStep < y + TBlockSize; yStep++)
            {
                for (auto xStep = x; xStep < x + TBlockSize; xStep++)
                {
                    int ly = std::min(yStep, ref_height - 1);
                    int lx = std::min(xStep, ref_width - 1);
                    m_pfTransmission[ly * ref_width + lx] = fTrans;
                }
            }
        }
    }
}


/*
    Function: NFTrsEstimation
    Description: Estiamte the transmission in the block. (COLOR)
        The algorithm use exhaustive searching method and its step size
        is sampled to 0.1

    Parameters:
        nStartx - top left point of a block
        nStarty - top left point of a block
        nWid - frame width
        nHei - frame height.
    Return:
        fOptTrs
 */
template <typename T>
float dehazing::NFTrsEstimationColor(const T* pnImageR, const T* pnImageG, const T* pnImageB, int nStartX, int nStartY, int ref_width, int ref_height)
{
    int nOutR, nOutG, nOutB;
    float fOptTrs;
    double dCost, dMinCost, dMean;

    int nEndX = std::min(nStartX + TBlockSize, ref_width);
    int nEndY = std::min(nStartY + TBlockSize, ref_height);

    int nNumberofPixels = (nEndY - nStartY) * (nEndX - nStartX) * 3;

    float fTrans = TransInit;
    int nTrans = (int)(((peak + 1) >> 1) / TransInit);

    for (auto nCounter = 0; nCounter < 7; nCounter++)
    {
        long long int nSumofSLoss = 0;
        int nLossCount = 0;
        long long int nSumofSquaredOuts = 0;
        long long int nSumofOuts = 0;

        int half_peak = ((peak + 1) >> 1);
        for (auto y = nStartY; y < nEndY; y++)
        {
            for (auto x = nStartX; x < nEndX; x++)
            {
                // (I-A)/t + A --> ((I-A) * k * ((peak + 1)/2) + A * ((peak+1)/2)) / ((peak+1)/2)
                nOutB = (((int)pnImageB[y * ref_width + x] - m_anAirlight[0]) * nTrans + half_peak * m_anAirlight[0]) / half_peak;
                nOutG = (((int)pnImageG[y * ref_width + x] - m_anAirlight[1]) * nTrans + half_peak * m_anAirlight[1]) / half_peak;
                nOutR = (((int)pnImageR[y * ref_width + x] - m_anAirlight[2]) * nTrans + half_peak * m_anAirlight[2]) / half_peak;

                if (nOutR > peak)
                {
                    nSumofSLoss += (nOutR - peak) * (nOutR - peak);
                    nLossCount++;
                }
                else if (nOutR < 0)
                {
                    nSumofSLoss += nOutR * nOutR;
                    nLossCount++;
                }
                if (nOutG > peak)
                {
                    nSumofSLoss += (nOutG - peak) * (nOutG - peak);
                    nLossCount++;
                }
                else if (nOutG < 0)
                {
                    nSumofSLoss += nOutG * nOutG;
                    nLossCount++;
                }
                if (nOutB > peak)
                {
                    nSumofSLoss += (nOutB - peak) * (nOutB - peak);
                    nLossCount++;
                }
                else if (nOutB < 0)
                {
                    nSumofSLoss += nOutB * nOutB;
                    nLossCount++;
                }
                nSumofSquaredOuts += nOutB * nOutB + nOutR * nOutR + nOutG * nOutG;;
                nSumofOuts += nOutR + nOutG + nOutB;
            }
        }

        dMean = (double)nSumofOuts / nNumberofPixels;
        dCost = Lambda1 * (double)nSumofSLoss / nNumberofPixels
                -((double)nSumofSquaredOuts / nNumberofPixels - dMean * dMean);

        if (nCounter == 0 || dMinCost > dCost)
        {
            dMinCost = dCost;
            fOptTrs = fTrans;
        }

        fTrans += 0.1f;
        nTrans = (int)(1.f / fTrans * ((peak + 1) >> 1));
    }
    return fOptTrs;
}


/*
    Function: AirlightEstimation
    Description: estimate the atmospheric light value in a hazy image.
                 it divides the hazy image into 4 sub-block and selects the optimal block,
                 which has minimum std-dev and maximum average value.
                 *Repeat* the dividing process until the size of sub-block is smaller than
                 pre-specified threshold value. Then, We select the most similar value to
                 the pure white.
                 IT IS A RECURSIVE FUNCTION.
    Parameter:
        imInput - input image
    Return:
        m_anAirlight: estimated atmospheric light value
 */
template <typename T>
void dehazing::AirlightEstimation(const T* src, int width, int height, int stride)
{
    int nMinDistance = (int)(peak * SQRT_3);

    int nMaxIndex;
    double dpScore[3];
    double dpMean[3] = { 0.0 };
    double dpStds[3] = { 0.0 };
    double variance[3] = { 0.0 };

    float afMean[4] = { 0.f };
    float afScore[4] = { 0.f };
    float nMaxScore = 0.f;

    // 4 sub-block
    int half_w = width / 2;
    int half_h = height / 2;

    T* iplUpperLeft  = new T[half_w * half_h * 3];
    T* iplUpperRight = new T[half_w * half_h * 3];
    T* iplLowerLeft  = new T[half_w * half_h * 3];
    T* iplLowerRight = new T[half_w * half_h * 3];

    memcpy(iplUpperLeft,  src, half_w * half_h * 3 * sizeof(T));
    memcpy(iplUpperRight, src + half_w * half_h * 3, half_w * half_h * 3 * sizeof(T));
    memcpy(iplLowerLeft,  src + half_w * half_h * 6, half_w * half_h * 3 * sizeof(T));
    memcpy(iplLowerRight, src + half_w * half_h * 9, half_w * half_h * 3 * sizeof(T));

    if (height * width > ABlockSize)
    {
        // compute the mean and std-dev in the sub-block
        T* iplR = new T[half_h * half_w];
        T* iplG = new T[half_h * half_w];
        T* iplB = new T[half_h * half_w];

        //////////////////////////////////
        // upper left sub-block
        for (auto j = 0; j < half_h; j++)
        {
            for (auto i = 0; i < half_w; i++)
            {
                const auto pos = (j * half_w + i) * 3;
                iplB[i] = iplUpperLeft[pos];
                iplG[i] = iplUpperLeft[pos + 1];
                iplR[i] = iplUpperLeft[pos + 2];
            }
        }

        meanStdDev(iplR, dpMean[0], dpStds[0], variance[0], half_w, half_h);
        meanStdDev(iplG, dpMean[1], dpStds[1], variance[1], half_w, half_h);
        meanStdDev(iplB, dpMean[2], dpStds[2], variance[2], half_w, half_h);
        // dpScore: mean - std-dev
        dpScore[0] = dpMean[0] - dpStds[0];
        dpScore[1] = dpMean[1] - dpStds[1];
        dpScore[2] = dpMean[2] - dpStds[2];

        afScore[0] = (float)(dpScore[0] + dpScore[1] + dpScore[2]);

        nMaxScore = afScore[0];
        nMaxIndex = 0;

        //////////////////////////////////
        // upper right sub-block
        for (auto j = 0; j < half_h; j++)
        {
            for (auto i = 0; i < half_w; i++)
            {
                const auto pos = (j * half_w + i) * 3;
                iplB[i] = iplUpperLeft[pos];
                iplG[i] = iplUpperLeft[pos + 1];
                iplR[i] = iplUpperLeft[pos + 2];
            }
        }

        meanStdDev(iplR, dpMean[0], dpStds[0], variance[0], half_w, half_h);
        meanStdDev(iplG, dpMean[1], dpStds[1], variance[1], half_w, half_h);
        meanStdDev(iplB, dpMean[2], dpStds[2], variance[2], half_w, half_h);

        dpScore[0] = dpMean[0] - dpStds[0];
        dpScore[1] = dpMean[1] - dpStds[1];
        dpScore[2] = dpMean[2] - dpStds[2];

        afScore[1] = (float)(dpScore[0] + dpScore[1] + dpScore[2]);

        if (afScore[1] > nMaxScore)
        {
            nMaxScore = afScore[1];
            nMaxIndex = 1;
        }

        //////////////////////////////////
        // lower left sub-block
        for (auto j = 0; j < half_h; j++)
        {
            for (auto i = 0; i < half_w; i++)
            {
                const auto pos = (j * half_w + i) * 3;
                iplB[i] = iplUpperLeft[pos];
                iplG[i] = iplUpperLeft[pos + 1];
                iplR[i] = iplUpperLeft[pos + 2];
            }
        }

        meanStdDev(iplR, dpMean[0], dpStds[0], variance[0], half_w, half_h);
        meanStdDev(iplG, dpMean[1], dpStds[1], variance[1], half_w, half_h);
        meanStdDev(iplB, dpMean[2], dpStds[2], variance[2], half_w, half_h);

        dpScore[0] = dpMean[0] - dpStds[0];
        dpScore[1] = dpMean[1] - dpStds[1];
        dpScore[2] = dpMean[2] - dpStds[2];

        afScore[2] = (float)(dpScore[0] + dpScore[1] + dpScore[2]);

        if (afScore[2] > nMaxScore)
        {
            nMaxScore = afScore[2];
            nMaxIndex = 2;
        }

        //////////////////////////////////
        // lower right sub-block
        for (auto j = 0; j < half_h; j++)
        {
            for (auto i = 0; i < half_w; i++)
            {
                const auto pos = (j * half_w + i) * 3;
                iplB[i] = iplUpperLeft[pos];
                iplG[i] = iplUpperLeft[pos + 1];
                iplR[i] = iplUpperLeft[pos + 2];
            }
        }

        meanStdDev(iplR, dpMean[0], dpStds[0], variance[0], half_w, half_h);
        meanStdDev(iplG, dpMean[1], dpStds[1], variance[1], half_w, half_h);
        meanStdDev(iplB, dpMean[2], dpStds[2], variance[2], half_w, half_h);

        dpScore[0] = dpMean[0] - dpStds[0];
        dpScore[1] = dpMean[1] - dpStds[1];
        dpScore[2] = dpMean[2] - dpStds[2];

        afScore[3] = (float)(dpScore[0] + dpScore[1] + dpScore[2]);

        if (afScore[3] > nMaxScore)
        {
            nMaxScore = afScore[3];
            nMaxIndex = 3;
        }

        // select the sub-block, which has maximum score
        switch (nMaxIndex)
        {
        case 0:
            AirlightEstimation(iplUpperLeft, half_w, half_h, stride / 2); break;
        case 1:
            AirlightEstimation(iplUpperRight, half_w, half_h, stride / 2); break;
        case 2:
            AirlightEstimation(iplLowerLeft, half_w, half_h, stride / 2); break;
        case 3:
            AirlightEstimation(iplLowerRight, half_w, half_h, stride / 2); break;
        }

        delete[] iplR;
        delete[] iplG;
        delete[] iplB;
    }
    else
    {
        // select the atmospheric light value in the sub-block
        for (auto j = 0; j < height; j++)
        {
            for (auto i = 0; i < width; i++)
            {
                const auto pos = (j * width + i) * 3;
                // peak-r, peak-g, peak-b
                int nDistance = (int)std::sqrt((float)(peak - src[pos]) * (peak - src[pos]) +
                                               (float)(peak - src[pos + 1]) * (peak - src[pos + 1]) +
                                               (float)(peak - src[pos + 2]) * (peak - src[pos + 2]));
                if (nMinDistance > nDistance)
                {
                    // atmospheric light value
                    nMinDistance = nDistance;
                    m_anAirlight[0] = src[pos];
                    m_anAirlight[1] = src[pos + 1];
                    m_anAirlight[2] = src[pos + 2];
                }
            }
        }
    }

    delete[] iplUpperLeft;
    delete[] iplUpperRight;
    delete[] iplLowerLeft;
    delete[] iplLowerRight;
}
