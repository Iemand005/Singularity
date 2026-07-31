#include "messagewidget.h"
#include "ui_messagewidget.h"

#include <QRegularExpression>

MessageWidget::MessageWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MessageWidget)
{
    ui->setupUi(this);

    connect(ui->prevBtn, &QPushButton::clicked, this, &MessageWidget::prevRequested);
    connect(ui->nextBtn, &QPushButton::clicked, this, &MessageWidget::nextRequested);
    connect(ui->regenerateBtn, &QPushButton::clicked, this, &MessageWidget::regenerateRequested);

    ui->versionBar->setVisible(false);
    ui->thinkContainer->setVisible(false);

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

void MessageWidget::setVersionInfo(size_t current, size_t total)
{
    if (total < 1) {
        ui->versionBar->setVisible(false);
        return;
    }
    ui->versionBar->setVisible(true);
    bool multi = total > 1;
    ui->prevBtn->setVisible(multi);
    ui->nextBtn->setVisible(multi);
    ui->versionLabel->setVisible(multi);
    ui->versionLabel->setText(QString("%1/%2").arg(current + 1).arg(total));
    ui->prevBtn->setEnabled(current > 0);
    ui->nextBtn->setEnabled(current + 1 < total);
    ui->regenerateBtn->setEnabled(true);
}

void MessageWidget::appendToken(const QString &token)
{
    m_hasStreamedContent = true;
    if (m_isThinking) {
        ui->thinkContent->setText(ui->thinkContent->text() + token);
        ui->thinkContainer->setVisible(true);
    } else {
        ui->textLabel->setText(ui->textLabel->text() + token);
    }
    emit sizeChanged();
}

void MessageWidget::appendTokenReasoning(const QString &token, bool thinking)
{
    m_hasStreamedContent = true;
    if (thinking) {
        ui->thinkContent->setText(ui->thinkContent->text() + token);
        ui->thinkContainer->setVisible(true);
    } else {
        ui->textLabel->setText(ui->textLabel->text() + token);
    }
    emit sizeChanged();
}

void MessageWidget::setThinking(bool thinking)
{
    if (m_isThinking == thinking) return;
    m_isThinking = thinking;
    if (thinking) {
        ui->thinkContainer->setVisible(true);
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
    QString normalContent;

    // Pattern: thinking markers like  text... (ellipsis) or  text... (Korean)
    // Start markers:  ... ...
    // End markers:   ... ...
    QRegularExpression re(QStringLiteral("<think>(.*?)</think>"), QRegularExpression::DotMatchesEverythingOption);
    int pos = 0;
    while ((pos = re.match(remaining, pos).capturedStart()) != -1) {
        QRegularExpressionMatch match = re.match(remaining, pos);
        if (!match.hasMatch()) break;

        int start = match.capturedStart();
        int end = match.capturedEnd();
        QString thinking = match.captured(1).trimmed();
        if (!thinking.isEmpty()) {
            thinkingContent += thinking + "\n\n";
        }

        // Remove the thinking block from normal content
        remaining.remove(start, end - start);
        pos = start;
    }

    // Also remove standalone thinking start/end markers
    // remaining = remaining.replace(QRegularExpression(QStringLiteral("...|...|...")), "");
    // remaining = remaining.replace(QRegularExpression(QStringLiteral("...|...|...")), "");

    if (!thinkingContent.isEmpty()) {
        ui->thinkContent->setText(thinkingContent.trimmed());
        ui->thinkContainer->setVisible(true);
    }
    ui->textLabel->setText(remaining.trimmed());
    m_everHadContent = true;
    emit sizeChanged();
}