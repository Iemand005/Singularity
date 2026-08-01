
#include <QString>

#include <TextGenOptions.hpp>
#include <Callbacks.h>

typedef std::function<void(const QString &token)> QTokenCallback;
typedef std::function<void(const QString &token, bool thinking)> QTokenReasoningCallback;
typedef std::function<void(const QString &error)> QErrorCallback;

struct QTextGenOptions : TextGenOptionsBase
{
    QTokenCallback onToken = nullptr;
    QTokenReasoningCallback onTokenReasoning = nullptr;
    ProgressCallback onInputEval = nullptr;
    ThinkStateChangedCallback onThinkStateChange = nullptr;
};

struct QAsyncTextGenOptions : QTextGenOptions {
    // FinishedTCallback<const TextGenResult &> onDone = nullptr;
    TextFinishCallback onDone = nullptr;
    QErrorCallback onError = nullptr;
};
