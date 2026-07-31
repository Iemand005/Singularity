#include "messagewidget.h"
#include "ui_messagewidget.h"

#include <QRegularExpression>

MessageWidget::MessageWidget(QWidget *parent)
    : QWidget(parent)
    , m_thinkToggle(nullptr)
    , m_thinkScroll(nullptr)
    , m_thinkContent(nullptr)
    , ui(new Ui::MessageWidget)
{
    ui->setupUi(this);

    m_thinkToggle = ui->thinkToggle;
    m_thinkScroll = ui->thinkScroll;
    m_thinkContent = ui->thinkContent;

    connect(ui->prevBtn, &QPushButton::clicked, this, &MessageWidget::prevRequested);
    connect(ui->nextBtn, &QPushButton::clicked, this, &MessageWidget::nextRequested);
    connect(ui->regenerateBtn, &QPushButton::clicked, this, &MessageWidget::regenerateRequested);

    hideVersionBar();
    hideThinking();

    connect(ui->thinkToggle, &QToolButton::toggled, this, [this](bool checked) {
        ui->thinkScroll->setVisible(checked);
        ui->thinkToggle->setText(checked ? QStringLiteral("\u25BC Hide thinking") : QStringLiteral("\u25B6 Show thinking"));
        emit sizeChanged();
    });
}

MessageWidget::~MessageWidget()
{
    delete ui;
}

void MessageWidget::hideThinking()
{
    if (ui->thinkToggle) ui->thinkToggle->setVisible(false);
    if (ui->thinkScroll) ui->thinkScroll->setVisible(false);
}

void MessageWidget::showThinking()
{
    if (ui->thinkToggle) ui->thinkToggle->setVisible(true);
    if (ui->thinkScroll) ui->thinkScroll->setVisible(true);
}

void MessageWidget::hideVersionBar()
{
    if (ui->prevBtn) ui->prevBtn->setVisible(false);
    if (ui->versionLabel) ui->versionLabel->setVisible(false);
    if (ui->nextBtn) ui->nextBtn->setVisible(false);
}

void MessageWidget::showVersionBar()
{
    if (ui->prevBtn) ui->prevBtn->setVisible(true);
    if (ui->versionLabel) ui->versionLabel->setVisible(true);
    if (ui->nextBtn) ui->nextBtn->setVisible(true);
}

void MessageWidget::setVersionInfo(size_t current, size_t total)
{
    if (total < 1) {
        hideVersionBar();
        return;
    }
    showVersionBar();
    bool multi = total > 1;
    if (ui->prevBtn) {
        ui->prevBtn->setVisible(multi);
        ui->prevBtn->setEnabled(current > 0);
    }
    if (ui->nextBtn) {
        ui->nextBtn->setVisible(multi);
        ui->nextBtn->setEnabled(current + 1 < total);
    }
    if (ui->versionLabel) {
        ui->versionLabel->setVisible(multi);
        ui->versionLabel->setText(QString("%1/%2").arg(current + 1).arg(total));
    }
    if (ui->regenerateBtn) ui->regenerateBtn->setEnabled(true);
}

void MessageWidget::appendToken(const QString &token)
{
    m_hasStreamedContent = true;
    // Strip thinking tags from streamed output
    QString cleanToken = token;
    cleanToken.remove(QRegularExpression(QStringLiteral("</?thinking\\b[^>]*>\\s*")));
    if (m_isThinking) {
        appendToThinking(cleanToken);
    } else {
        if (ui->textLabel)
            ui->textLabel->setText(ui->textLabel->text() + cleanToken);
    }
    emit sizeChanged();
}

void MessageWidget::appendTokenReasoning(const QString &token, bool thinking)
{
    m_hasStreamedContent = true;
    if (thinking) {
        appendToThinking(token);
    } else {
        if (ui->textLabel)
            ui->textLabel->setText(ui->textLabel->text() + token);
    }
    emit sizeChanged();
}

void MessageWidget::appendToThinking(const QString &token)
{
    if (m_thinkContent) {
        QString cleanToken = token;
        cleanToken.remove(QRegularExpression(QStringLiteral("</?thinking\\b[^>]*>\\s*")));
        m_thinkContent->setText(m_thinkContent->text() + cleanToken);
    }
    showThinking();
}

void MessageWidget::setThinking(bool thinking)
{
    if (m_isThinking == thinking) return;
    m_isThinking = thinking;
    if (thinking) {
        showThinking();
    }
    emit sizeChanged();
}

void MessageWidget::finish()
{
    m_everHadContent = true;
    emit sizeChanged();
}

void MessageWidget::setContent(const QString &text)
{
    if (m_everHadContent || m_hasStreamedContent) return;

    // Parse thinking blocks from stored content
    QString remaining = text;
    QString thinkingContent;

    // Pattern: thinking markers  text...
    QRegularExpression re(QStringLiteral("</?thinking\\b[^>]*>\\s*(.*?)(?:<\\s*/\\s*thinking\\s*>|$)"), QRegularExpression::DotMatchesEverythingOption);
    QRegularExpression thinkTagRe(QStringLiteral("</?thinking\\b[^>]*>"));
    remaining.remove(thinkTagRe);

    int pos = 0;
    while (pos < remaining.size()) {
        QRegularExpressionMatch match = re.match(remaining, pos);
        if (!match.hasMatch()) break;

        int start = match.capturedStart();
        int end = match.capturedEnd();
        QString thinking = match.captured(1).trimmed();
        if (!thinking.isEmpty()) {
            thinkingContent += thinking + "\n\n";
        }

        remaining.remove(start, end - start);
        pos = start;
    }

    if (!thinkingContent.isEmpty()) {
        if (m_thinkContent) m_thinkContent->setText(thinkingContent.trimmed());
        showThinking();
    }
    if (ui->textLabel)
        ui->textLabel->setText(remaining.trimmed());
    m_everHadContent = true;
    emit sizeChanged();
}
