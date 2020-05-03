#include "Math/MathHelpers.h"
#include <string>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>


namespace eg {

void GetFloatValues(const char *pValue, float *floatValues, size_t numValues)
{
    if (!pValue)
    {
        // \log error string is NULL
        return;
    }

    std::string values(pValue);
    boost::tokenizer<> tok(values);
    size_t index = 0;
    try
    {
        for (boost::tokenizer<>::iterator it = tok.begin(); it != tok.end(); ++it, index++)
        {
            if (index == numValues)
            {
                // in some case, user might have passed longer pValue values
                // then numValues passed (floatValues size)
                break;
            }

            floatValues[index] = boost::lexical_cast<float>(*it);
        }
    }
    catch (const boost::bad_lexical_cast& /*e*/)
    {
        // \log error that we could not convert token string into float
    }

    for (size_t i = index; i < numValues; i++)
    {
        floatValues[i] = 0.0f;
    }
}


} // namespace eg
