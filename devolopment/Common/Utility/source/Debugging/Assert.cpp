// see README.md for usage instructions.
// (‑●‑●)> released under the WTFPL v2 license, by Gregory Pakosz (@gpakosz)
#include "Platform/WindowsLean.h"
#include "Platform/Support.h"
#include "Debugging/Assert.h"
#include "Fmt/format.h"

#include <cstdio>  // fprintf() and vsnprintf()
#include <cstring>
#include <cstdarg> // va_start() and va_end()
#include <cstdlib> // abort()

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(__ANDROID__) || defined(ANDROID)
#include <android/log.h>
#if !defined(YAGET_ASSERT_LOG_TAG)
#define YAGET_ASSERT_LOG_TAG "YAGET_ASSERT"
#endif
#endif

//#define YAGET_ASSERT_LOG_FILE "/tmp/assert.txt"
//#define YAGET_ASSERT_LOG_FILE_TRUNCATE

// malloc and free are only used by AssertionException implemented in terms of
// short string optimization.
// However, no memory allocation happens if
// YAGET_ASSERT_EXCEPTION_MESSAGE_BUFFER_SIZE == YAGET_ASSERT_MESSAGE_BUFFER_SIZE
// which is the default.
#if !defined(YAGET_ASSERT_MALLOC)
#define YAGET_ASSERT_MALLOC(size) malloc(size)
#endif

#if !defined(YAGET_ASSERT_FREE)
#define YAGET_ASSERT_FREE(p) free(p)
#endif

#if !defined(YAGET_ASSERT_MESSAGE_BUFFER_SIZE)
#  define YAGET_ASSERT_MESSAGE_BUFFER_SIZE YAGET_ASSERT_EXCEPTION_MESSAGE_BUFFER_SIZE
#endif

#if !defined(YAGET_ASSERT_ABORT)
#define YAGET_ASSERT_ABORT abort
#endif

#pragma warning (disable : 4996)  // warning C4996: 'freopen': This function or variable may be unsafe. Consider using freopen_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.

#if YAGET_ASSERT_ENABLED == 1
    #pragma message("======== Yaget Asserts Included ========")
#else
    #pragma message("======== Yaget Asserts NOT Included ========")
#endif // YAGET_ASSERT_ENABLED


namespace
{
    namespace AssertLevel = yaget::assert::implementation::AssertLevel;
    namespace AssertAction = yaget::assert::implementation::AssertAction;

    typedef int (*printHandler)(FILE* out, int, const char* format, ...);

#if defined(YAGET_ASSERT_LOG_FILE) && defined(YAGET_ASSERT_LOG_FILE_TRUNCATE)
  struct LogFileTruncate
  {
    LogFileTruncate()
    {
      if (FILE* f = fopen(YAGET_ASSERT_LOG_FILE, "w"))
        fclose(f);
    }
  };

  static LogFileTruncate truncate;
#endif

    int print(FILE* out, int level, const char* format, ...)
    {
        va_list args;
        int count;

        va_start(args, format);
        count = vfprintf(out, format, args);
        fflush(out);
        va_end(args);

#if defined(YAGET_ASSERT_LOG_FILE)
    struct Local
    {
      static void log(const char* format, va_list args)
      {
        if (FILE* f = fopen(YAGET_ASSERT_LOG_FILE, "a"))
        {
          vfprintf(f, format, args);
          fclose(f);
        }
      }
    };

    va_start(args, format);
    Local::log(format, args);
    va_end(args);
#endif

#if defined(_WIN32)
        char buffer[YAGET_ASSERT_MESSAGE_BUFFER_SIZE];
        va_start(args, format);
        vsnprintf(buffer, YAGET_ASSERT_MESSAGE_BUFFER_SIZE, format, args);
        ::OutputDebugStringA(buffer);
        va_end(args);
#endif

#if defined(__ANDROID__) || defined(ANDROID)
    int priority = ANDROID_LOG_VERBOSE;

    if (level >= AssertLevel::Debug)
      priority = ANDROID_LOG_DEBUG;
    else if (level >= AssertLevel::Warning)
      priority = ANDROID_LOG_WARN;
    else if (level >= AssertLevel::Error)
      priority = ANDROID_LOG_ERROR;
    else if (level >= AssertLevel::Fatal)
      priority = ANDROID_LOG_FATAL;

    va_start(args, format);
    __android_log_vprint(priority, YAGET_ASSERT_LOG_TAG, format, args);
    va_start(args, format);
#else
        YAGET_ASSERT_UNUSED(level);
#endif

        return count;
    }

    int formatLevel(int level, const char* expression, FILE* out, printHandler print)
    {
        const char* levelstr = 0;

        switch (level)
        {
            case AssertLevel::Debug:
                levelstr = "DEBUG";
                break;
            case AssertLevel::Warning:
                levelstr = "WARNING";
                break;
            case AssertLevel::Error:
                levelstr = "ERROR";
                break;
            case AssertLevel::Fatal:
                levelstr = "FATAL";
                break;

            default:
                break;
        }

        if (levelstr)
            return print(out, level, "Assertion '%s' failed (%s)\n", expression, levelstr);
        else
            return print(out, level, "Assertion '%s' failed (level = %d)\n", expression, level);
    }

    AssertAction::AssertAction _defaultHandler(const char* file,
                                               int line,
                                               const char* function,
                                               const char* expression,
                                               int level,
                                               const char* message)
    {
#if defined(_WIN32)
        if (::GetConsoleWindow() == NULL)
        {
            if (::AllocConsole())
            {
                (void)freopen("CONIN$", "r", stdin);
                (void)freopen("CONOUT$", "w", stdout);
                (void)freopen("CONOUT$", "w", stderr);

                SetFocus(::GetConsoleWindow());
            }
        }
#endif

        formatLevel(level, expression, stderr, reinterpret_cast<printHandler>(print));
        print(stderr, level, "  %s(%d): function: %s\n", file, line, function);

        if (message)
            print(stderr, level, "  with message: '%s'\n\n", message);

        if (level < AssertLevel::Debug)
        {
            return AssertAction::None;
        }
        else if (AssertLevel::Debug <= level && level < AssertLevel::Error)
        {
#if (!TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR) && (!defined(__ANDROID__) && !defined(ANDROID)) || defined(YAGET_ASSERT_DEFAULT_HANDLER_STDIN)
            for (;;)
            {
#if defined(YAGET_ASSERT_DISABLE_IGNORE_LINE)
        fprintf(stderr, "Press (I)gnore / Ignore (A)ll / (D)ebug / A(b)ort: ");
#else
                fprintf(stderr, "Press (I)gnore / Ignore (F)orever / Ignore (A)ll / (D)ebug / A(b)ort: ");
#endif
                fflush(stderr);

                char buffer[256];
                if (!fgets(buffer, sizeof(buffer), stdin))
                {
                    clearerr(stdin);
                    fprintf(stderr, "\n");
                    fflush(stderr);
                    continue;
                }

                // we eventually skip the leading spaces but that's it
                char input[2] = {'b', 0};
                if (sscanf(buffer, " %1[a-zA-Z] ", input) != 1)
                    continue;

                switch (*input)
                {
                    case 'b':
                    case 'B':
                        return AssertAction::Abort;

                    case 'd':
                    case 'D':
                        return AssertAction::Break;

                    case 'i':
                    case 'I':
                        return AssertAction::Ignore;

#  if !defined(YAGET_ASSERT_DISABLE_IGNORE_LINE)
                    case 'f':
                    case 'F':
                        return AssertAction::IgnoreLine;
#  endif

                    case 'a':
                    case 'A':
                        return AssertAction::IgnoreAll;

                    default:
                        break;
                }
            }
#else
      return AssertAction::Break;
#endif
        }
        else if (AssertLevel::Error <= level && level < AssertLevel::Fatal)
        {
            return AssertAction::Throw;
        }

        return AssertAction::Abort;
    }

    void _throw(const char* file,
                int line,
                const char* function,
                const char* expression,
                const char* message)
    {
        using yaget::assert::implementation::throwException;
        throwException(yaget::assert::AssertionException(file, line, function, expression, message));
    }
}

namespace yaget
{
    namespace assert
    {
        AssertionException::AssertionException(const char* file,
                                               int line,
                                               const char* function,
                                               const char* expression,
                                               const char* message)
            : _file(file)
            , _line(line)
            , _function(function)
            , _expression(expression)
            , _heap(YAGET_ASSERT_NULLPTR)
        {
            if (!message)
            {
                strncpy(_stack, "", size);
                return;
            }

            size_t length = strlen(message);

            if (length < size) // message is short enough for the stack buffer
            {
                strncpy(_stack, message, length);
                strncpy(_stack + length, "", size - length); // pad with 0
            }
            else // allocate storage on the heap
            {
                _heap = static_cast<char*>(YAGET_ASSERT_MALLOC(sizeof(char) * (length + 1)));

                if (!_heap) // allocation failed
                {
                    strncpy(_stack, message, size - 1); // stack fallback, truncate :/
                    _stack[size - 1] = 0;
                }
                else
                {
                    strncpy(_heap, message, length); // copy the message
                    _heap[length] = 0;
                    _stack[size - 1] = 1; // mark the stack
                }
            }
        }

        AssertionException::AssertionException(const AssertionException& rhs)
            : _file(rhs._file)
            , _line(rhs._line)
            , _function(rhs._function)
            , _expression(rhs._expression)
        {
            const char* message = rhs.what();
            size_t length = strlen(message);

            if (length < size) // message is short enough for the stack buffer
            {
                strncpy(_stack, message, size); // pad with 0
            }
            else // allocate storage on the heap
            {
                _heap = static_cast<char*>(YAGET_ASSERT_MALLOC(sizeof(char) * (length + 1)));

                if (!_heap) // allocation failed
                {
                    strncpy(_stack, message, size - 1); // stack fallback, truncate :/
                    _stack[size - 1] = 0;
                }
                else
                {
                    strncpy(_heap, message, length); // copy the message
                    _heap[length] = 0;
                    _stack[size - 1] = 1; // mark the stack
                }
            }
        }

        AssertionException::~AssertionException() YAGET_ASSERT_EXCEPTION_NO_THROW
        {
            if (_stack[size - 1])
            YAGET_ASSERT_FREE(_heap);

            _heap = YAGET_ASSERT_NULLPTR; // in case the exception object is destroyed twice
            _stack[size - 1] = 0;
        }

        AssertionException& AssertionException::operator =(const AssertionException& rhs)
        {
            if (&rhs == this)
                return *this;

            const char* message = rhs.what();
            size_t length = strlen(message);

            if (length < size) // message is short enough for the stack buffer
            {
                if (_stack[size - 1])
                YAGET_ASSERT_FREE(_heap);

                strncpy(_stack, message, size);
            }
            else // allocate storage on the heap
            {
                if (_stack[size - 1])
                {
                    size_t _length = strlen(_heap);

                    if (length <= _length)
                    {
                        strncpy(_heap, message, _length); // copy the message, pad with 0
                        return *this;
                    }
                    else
                    {
                        YAGET_ASSERT_FREE(_heap);
                    }
                }

                _heap = static_cast<char*>(YAGET_ASSERT_MALLOC(sizeof(char) * (length + 1)));

                if (!_heap) // allocation failed
                {
                    strncpy(_stack, message, size - 1); // stack fallback, truncate :/
                    _stack[size - 1] = 0;
                }
                else
                {
                    strncpy(_heap, message, length); // copy the message
                    _heap[length] = 0;
                    _stack[size - 1] = 1; // mark the stack
                }
            }

            _file = rhs._file;
            _line = rhs._line;
            _function = rhs._function;
            _expression = rhs._expression;

            return *this;
        }

        const char* AssertionException::what() const YAGET_ASSERT_EXCEPTION_NO_THROW
        {
            return _stack[size - 1] ? _heap : _stack;
        }

        namespace implementation
        {
            namespace
            {
                bool _ignoreAll = false;
            }

            void ignoreAllAsserts(bool value)
            {
                _ignoreAll = value;
            }

            bool ignoreAllAsserts()
            {
                return _ignoreAll;
            }

            namespace
            {
                AssertHandler _handler = _defaultHandler;
            }

            AssertHandler setAssertHandler(AssertHandler handler)
            {
                AssertHandler previous = _handler;

                _handler = handler ? handler : _defaultHandler;

                return previous;
            }

            AssertAction::AssertAction handleAssert(const char* file,
                                                    int line,
                                                    const char* function,
                                                    const char* expression,
                                                    int level,
                                                    bool* ignoreLine,
                                                    const char* message, ...)
            {
                char message_[YAGET_ASSERT_MESSAGE_BUFFER_SIZE] = {0};

                if (message)
                {
                    va_list args;
                    va_start(args, message);
                    vsnprintf(message_, YAGET_ASSERT_MESSAGE_BUFFER_SIZE, message, args);
                    va_end(args);

                    message = message_;
                }

                if (yaget::platform::IsDebuggerAttached())
                {
                    std::string errorMessage = fmt::format("{}({}) : [BEGUG ERROR] {}", file, line, message);
                    yaget::platform::DebuggerOutput(errorMessage);
                    yaget::platform::DebuggerBreak();
                }

                AssertAction::AssertAction action = _handler(file, line, function, expression, level, message);

                switch (action)
                {
                    case AssertAction::Abort:
                        YAGET_ASSERT_ABORT();

#if !defined(YAGET_ASSERT_DISABLE_IGNORE_LINE)
                    case AssertAction::IgnoreLine:
                        *ignoreLine = true;
                        break;
#else
      YAGET_ASSERT_UNUSED(ignoreLine);
#endif // YAGET_ASSERT_DISABLE_IGNORE_LINE

                    case AssertAction::IgnoreAll:
                        ignoreAllAsserts(true);
                        break;

                    case AssertAction::Throw:
                        _throw(file, line, function, expression, message);
                        break;

                    case AssertAction::Ignore:
                    case AssertAction::Break:
                    case AssertAction::None:
                    default:
                        return action;
                }

                return AssertAction::None;
            }
        } // namespace implementation
    } // namespace assert
} // namespace yaget
