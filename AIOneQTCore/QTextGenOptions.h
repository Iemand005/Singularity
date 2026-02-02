
#include <QString>

#include <TextGenerationOptions.hpp>

typedef std::function<void(const QString &token)> QTokenCallback;

struct QTextGenOptions : TextGenOptionsBase
{
    QTokenCallback onToken = nullptr;
    ProgressCallback onInputEval = nullptr;
};

struct QAsyncTextGenOptions : QTextGenOptions {
    FinishCallback onDone = nullptr;
};
