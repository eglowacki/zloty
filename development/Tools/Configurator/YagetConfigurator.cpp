// YagetConfigurator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "YagetCore.h"
#include <iostream>


namespace yaget::ylog
{
  yaget::Strings GetRegisteredTags()
  {
      yaget::Strings tags =
      {
          #include "Logger/CoreLogTags.h"
      };

      return tags;
  }
} // namespace yaget::ylog


YAGET_BRAND_NAME_F("Beyond Limits")

int main()
{
    using namespace yaget;

    /*auto result =*/ system::InitializeSetup();
}
