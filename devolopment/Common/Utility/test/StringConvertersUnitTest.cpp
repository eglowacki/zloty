#include "StringHelpers.h"
#include "UnitTest++.h"
#include "Streams/Guid.h"

//CHECK_EQUAL(expected, actual);

namespace
{
    template<typename T>
    struct Limits
    {
        static T Min() { return std::numeric_limits<T>::min(); }
        static T Max() { return std::numeric_limits<T>::max(); }
        static T Init() { return T{}; }
    };

    template<>
    struct Limits<float>
    {
        static float Min() { return std::numeric_limits<float>::min() + 100.0f; }
        static float Max() { return std::numeric_limits<float>::max() - 100.0f; }
        static float Init() { return 0.0f; }
    };

    template<>
    struct Limits<std::string>
    {
        static std::string Min() { return ""; }
        static std::string Max() { return "qwertyuiop12344"; }
        static std::string Init() { return "Hello World!"; }
    };

    template<>
    struct Limits<yaget::Guid>
    {
        static yaget::Guid Min() { return ""; }
        static yaget::Guid Max() { return "qwertyuiop12344"; }
        static yaget::Guid Init() { return "7bcd757f-5b10-4f9b-af69-1a1f226f3b3e"; }
    };

    template<>
    struct Limits<math3d::Vector3>
    {
        static math3d::Vector3 Min() { return { -10.0f, -10.0f, -10.0f }; }
        static math3d::Vector3 Max() { return { 10.0f, 10.0f, 10.0f }; }
        static math3d::Vector3 Init() { return { 0.0f, 0.0f, 0.0f }; }
    };

    template<>
    struct Limits<math3d::Vector2>
    {
        static math3d::Vector2 Min() { return { -10.0f, -10.0f }; }
        static math3d::Vector2 Max() { return { 10.0f, 10.0f }; }
        static math3d::Vector2 Init() { return { 0.0f, 0.0f }; }
    };

    template<>
    struct Limits<math3d::Color>
    {
        static math3d::Color Min() { return { -10.0f, -10.0f, -10.0f, 1.0f }; }
        static math3d::Color Max() { return { 10.0f, 10.0f, 10.0f, 1.0f }; }
        static math3d::Color Init() { return { 0.0f, 0.0f, 0.0f, 1.0f }; }
    };

    template<>
    struct Limits<yaget::Strings>
    {
        static yaget::Strings Min() { return yaget::Strings{ }; }
        static yaget::Strings Max() { return yaget::Strings{ "Element1" }; }
        static yaget::Strings Init() { return yaget::Strings{ "Element1", "Element2" }; }
    };

    template<typename V1, typename V2>
    struct Limits<std::pair<V1, V2>>
    {
        static std::pair<V1, V2> Min() { return std::pair<V1, V2>{ Limits<V1>::Min(), Limits<V2>::Min() }; }
        static std::pair<V1, V2> Max() { return std::pair<V1, V2>{ Limits<V1>::Max(), Limits<V2>::Max() }; }
        static std::pair<V1, V2> Init() { return std::pair<V1, V2>{ Limits<V1>::Init(), Limits<V2>::Init() }; }
    };

    template<>
    struct Limits<const char*>
    {
        static const char* Min() { return ""; }
        static const char* Max() { return "Element1"; }
        static const char* Init() { return "Element2"; }
    };

    template<typename T, typename L = Limits<T>>
    void VerifyConversion()
    {
        using Limit = L;
        T expectedValue = Limit::Init();
        std::string toString = yaget::conv::Convertor<T>::ToString(expectedValue);
        auto convertedValue = yaget::conv::Convertor<T>::FromString(toString.c_str());
        CHECK_EQUAL(expectedValue, convertedValue);

        expectedValue = Limit::Min();
        toString = yaget::conv::Convertor<T>::ToString(expectedValue);
        convertedValue = yaget::conv::Convertor<T>::FromString(toString.c_str());
        CHECK_EQUAL(expectedValue, convertedValue);

        expectedValue = Limit::Max();
        toString = yaget::conv::Convertor<T>::ToString(expectedValue);
        convertedValue = yaget::conv::Convertor<T>::FromString(toString.c_str());
        CHECK_EQUAL(expectedValue, convertedValue);
    }

} // namespace


std::ostream &operator<<(std::ostream &s, const DirectX::SimpleMath::Vector3& vector)
{
    return s << vector.x << " " << vector.y << " " << vector.z;
}

std::ostream &operator<<(std::ostream &s, const DirectX::SimpleMath::Vector2& vector)
{
    return s << vector.x << " " << vector.y;
}

std::ostream &operator<<(std::ostream &s, const DirectX::SimpleMath::Color& vector)
{
    return s << vector.x << " " << vector.y << " " << vector.z << " " << vector.w;
}

template<typename V1, typename V2>
std::ostream &operator<<(std::ostream &s, const std::pair<V1, V2>& vector)
{
    return s << vector.first << "x" << vector.second;
}

std::ostream &operator<<(std::ostream &s, const yaget::Strings& vector)
{
    for (const auto& v : vector)
    {
        s << " " << v;
    }
    return s;
}

TEST(BaseTypeConverters)
{
    VerifyConversion<char>();
    VerifyConversion<uint8_t>();
    VerifyConversion<uint32_t>();
    VerifyConversion<int>();
    VerifyConversion<size_t>();
    VerifyConversion<bool>();
    VerifyConversion<float>();
    VerifyConversion<std::string>();
    VerifyConversion<yaget::Guid>();
    VerifyConversion<math3d::Vector3>();
    VerifyConversion<math3d::Vector2>();
    VerifyConversion<math3d::Color>();
    VerifyConversion<yaget::Strings>();
    VerifyConversion<std::pair<int, int>>();
    VerifyConversion<std::pair<float, float>>();
    VerifyConversion<std::pair<uint32_t, bool>>();
    VerifyConversion<const char*>();
}
