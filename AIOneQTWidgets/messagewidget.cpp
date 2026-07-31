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

    ui->thinkToggle->setChecked(false);
    ui->thinkToggle->setText(QStringLiteral("\u25B6 Show thinking"));
    ui->thinkScroll->setMaximumHeight(0);
    ui->thinkScroll->setMinimumHeight(0);

    hideVersionBar();
    hideThinking();

    connect(ui->thinkToggle, &QToolButton::toggled, this, [this](bool checked) {
        ui->thinkToggle->setText(checked ? QStringLiteral("\u25BC Hide thinking") : QStringLiteral("\u25B6 Show thinking"));
        animateThinking(checked);
    });

    if (parentWidget()) {
        m_eventWatchParent = parentWidget();
        m_eventWatchParent->installEventFilter(this);
    }
}

MessageWidget::~MessageWidget()
{
    stopThinkAnimation();
    delete ui;
}

bool MessageWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_eventWatchParent && event->type() == QEvent::Resize && m_thinkExpanded && ui->thinkScroll) {
        int target = thinkTargetHeight();
        ui->thinkScroll->setMaximumHeight(target);
        ui->thinkScroll->setMinimumHeight(target);
        emit sizeChanged();
    }
    return QWidget::eventFilter(watched, event);
}

QSize MessageWidget::sizeHint() const
{
    return contentSize();
}

QSize MessageWidget::minimumSizeHint() const
{
    return contentSize();
}

void MessageWidget::hideThinking()
{
    if (ui->thinkToggle) ui->thinkToggle->setVisible(false);
    if (ui->thinkScroll) ui->thinkScroll->setVisible(false);
}

void MessageWidget::showThinking()
{
    if (ui->thinkToggle) ui->thinkToggle->setVisible(true);
    if (ui->thinkScroll) {
        ui->thinkScroll->setVisible(true);
        if (!m_thinkExpanded) {
            ui->thinkScroll->setMaximumHeight(0);
            ui->thinkScroll->setMinimumHeight(0);
        }
    }
}

void MessageWidget::animateThinking(bool expand)
{
    m_thinkExpanded = expand;
    stopThinkAnimation();

    int target = expand ? thinkTargetHeight() : 0;

    if (expand) {
        // Size the item to the full expanded height right away so the scroll
        // area has room to grow without relayouting the whole list each frame.
        ui->thinkScroll->setMaximumHeight(target);
        ui->thinkScroll->setMinimumHeight(target);
        emit sizeChanged();
    }

    m_thinkAnimMax = new QPropertyAnimation(ui->thinkScroll, "maximumHeight", this);
    m_thinkAnimMax->setDuration(250);
    m_thinkAnimMax->setEasingCurve(QEasingCurve::InOutCubic);
    m_thinkAnimMax->setStartValue(expand ? 0 : ui->thinkScroll->maximumHeight());
    m_thinkAnimMax->setEndValue(target);

    connect(m_thinkAnimMax, &QPropertyAnimation::valueChanged, this, [this](const QVariant &v) {
        ui->thinkScroll->setMinimumHeight(v.toInt());
    });
    connect(m_thinkAnimMax, &QPropertyAnimation::finished, this, [this, expand]() {
        stopThinkAnimation();
        if (!expand) {
            ui->thinkScroll->setMaximumHeight(0);
            ui->thinkScroll->setMinimumHeight(0);
        }
        emit sizeChanged();
    });

    m_thinkAnimMax->start();
}

void MessageWidget::stopThinkAnimation()
{
    if (m_thinkAnimMax) {
        m_thinkAnimMax->stop();
        m_thinkAnimMax->deleteLater();
        m_thinkAnimMax = nullptr;
    }
}

int MessageWidget::thinkTargetHeight() const
{
    int parentH = parentWidget() ? parentWidget()->height() : 400;
    return qMax(80, parentH / 2);
}

QSize MessageWidget::contentSize() const
{
    QWidget *parent = parentWidget();
    int width = parent ? parent->width() : 400;
    return QSize(width, contentHeightForWidth(width));
}

int MessageWidget::contentHeightForWidth(int width) const
{
    int labelWidth = qMax(10, width - 16);

    int thinkH = 0;
    if (ui->thinkToggle && ui->thinkToggle->isVisible())
        thinkH += ui->thinkToggle->sizeHint().height();
    if (ui->thinkScroll && ui->thinkScroll->isVisible())
        thinkH += ui->thinkScroll->maximumHeight();

    int textH = 0;
    if (ui->textLabel && ui->textLabel->isVisible()) {
        textH = ui->textLabel->heightForWidth(labelWidth);
        if (textH <= 0) textH = ui->textLabel->sizeHint().height();
    }

    int versionH = ui->regenerateBtn ? ui->regenerateBtn->sizeHint().height() : 30;

    int visibleRows = (thinkH > 0 ? 1 : 0) + 1 + 1;
    int spacing = (visibleRows - 1) * 4;

    return qMax(40, 16 + spacing + thinkH + textH + versionH);
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
    processToken(token);
    emit sizeChanged();
}

void MessageWidget::appendTokenReasoning(const QString &token, bool)
{
    m_hasStreamedContent = true;
    processToken(token);
    emit sizeChanged();
}

void MessageWidget::appendToThinking(const QString &token)
{
    if (m_thinkContent) {
        m_thinkContent->setText(m_thinkContent->text() + token);
    }
    showThinking();
}

void MessageWidget::processToken(const QString &token)
{
    m_tagBuffer += token;

    int pos = 0;
    while (pos < m_tagBuffer.size()) {
        int openTag = m_tagBuffer.indexOf("<think", pos);
        int closeTag = m_tagBuffer.indexOf("</think", pos);

        int nextTag;
        bool isOpen;
        if (openTag != -1 && (closeTag == -1 || openTag < closeTag)) {
            nextTag = openTag;
            isOpen = true;
        } else if (closeTag != -1) {
            nextTag = closeTag;
            isOpen = false;
        } else {
            routeText(m_tagBuffer.mid(pos));
            m_tagBuffer.clear();
            break;
        }

        routeText(m_tagBuffer.mid(pos, nextTag - pos));

        int tagEnd = m_tagBuffer.indexOf('>', nextTag);
        if (tagEnd == -1) {
            m_tagBuffer = m_tagBuffer.mid(nextTag);
            break;
        }

        setThinking(isOpen);
        m_tagBuffer.remove(0, tagEnd + 1);
        pos = 0;
    }
}

void MessageWidget::routeText(const QString &text)
{
    if (text.isEmpty()) return;
    if (m_isThinking) {
        appendToThinking(text);
    } else if (ui->textLabel) {
        ui->textLabel->setText(ui->textLabel->text() + text);
    }
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
    QString remaining = text;
    QString thinkingContent;

    int pos = 0;
    while (pos < remaining.size()) {
        int openIdx = remaining.indexOf("<think", pos);
        if (openIdx == -1) break;
        int openEnd = remaining.indexOf('>', openIdx);
        if (openEnd == -1) break;

        int closeIdx = remaining.indexOf("</think", openEnd);
        if (closeIdx != -1) {
            int closeEnd = remaining.indexOf('>', closeIdx);
            if (closeEnd == -1) break;

            QString think = remaining.mid(openEnd + 1, closeIdx - openEnd - 1).trimmed();
            if (!think.isEmpty())
                thinkingContent += think + "\n\n";

            remaining.remove(openIdx, closeEnd + 1 - openIdx);
            pos = openIdx;
        } else {
            QString think = remaining.mid(openEnd + 1).trimmed();
            if (!think.isEmpty())
                thinkingContent += think;

            remaining.remove(openIdx, remaining.size() - openIdx);
            break;
        }
    }

    if (!thinkingContent.isEmpty()) {
        if (m_thinkContent) m_thinkContent->setText(thinkingContent.trimmed());
        showThinking();
    }
    if (ui->textLabel)
        ui->textLabel->setText(remaining.trimmed());
    m_everHadContent = true;
    m_tagBuffer.clear();
    emit sizeChanged();
}
