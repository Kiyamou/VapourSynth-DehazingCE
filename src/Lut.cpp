#include <cmath>

#include "DehazingCE.hpp"

/*
    Function: MakeExpLUT
    Description: Make a Look Up Table(LUT) for applying previous information.

    Return:
        ExpLUT - output table
*/
void dehazing::MakeExpLUT()
{
    for (auto i = 0; i < peak + 1; i++)
    {
        ExpLUT[i] = exp(-(i * i) / 10.f);
    }
}

/*
    Function: GuideLUTMaker
    Description: Make a Look Up Table(LUT) for guided filtering

    Return:
        m_pfGuidedLUT - output table
*/
void dehazing::GuideLUTMaker()
{
    for (auto nX = 0; nX < GBlockSize / 2; nX++)
    {
        for (auto nY = 0; nY < GBlockSize / 2; nY++)
        {
            m_pfGuidedLUT[nY * GBlockSize + nX] =
                exp(-((nX - GBlockSize / 2 + 1) * (nX - GBlockSize / 2 + 1) +
                (nY - GBlockSize / 2 + 1) * (nY - GBlockSize / 2 + 1)) / (2 * GSigma * GSigma));

            m_pfGuidedLUT[(GBlockSize - nY - 1) * GBlockSize + nX] =
                exp(-((nX - GBlockSize / 2 + 1) * (nX - GBlockSize / 2 + 1) +
                (nY - GBlockSize / 2 + 1) * (nY - GBlockSize / 2 + 1)) / (2 * GSigma * GSigma));

            m_pfGuidedLUT[nY * GBlockSize + GBlockSize - nX - 1] =
                exp(-((nX - GBlockSize / 2 + 1) * (nX - GBlockSize / 2 + 1) +
                (nY - GBlockSize / 2 + 1) * (nY - GBlockSize / 2 + 1)) / (2 * GSigma * GSigma));

            m_pfGuidedLUT[(GBlockSize - nY - 1) * GBlockSize + GBlockSize - nX - 1] =
                exp(-((nX - GBlockSize / 2 + 1) * (nX - GBlockSize / 2 + 1) +
                (nY - GBlockSize / 2 + 1) * (nY - GBlockSize / 2 + 1)) / (2 * GSigma * GSigma));
        }
    }
}

/*
    Function: GammaLUTMaker
    Description: Make a Look Up Table(LUT) for gamma correction

    parameter:
        fParameter - gamma value.
    Return:
        m_pucGammaLUT - output table

*/
void dehazing::GammaLUTMaker(float fParameter)
{
    for (auto i = 0; i < peak + 1; i++)
    {
        m_pucGammaLUT[i] = pow((i / (float)peak), 1.f / fParameter) * (float)peak;
    }
}
