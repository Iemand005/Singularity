#include "messagewidget.h"

MessageWidget::MessageWidget(QWidget *parent)
    : QWidget(parent)
    , m_layout(new QVBoxLayout(this))
{
    m_layout->setContentsMargins(0, 2, 0, 2);
    m_layout->setSpacing(2);
}

void MessageWidget::appendToken(const QString &token)
{
    m_pending += token;
    processBuffer();
    emit sizeChanged();
}

void MessageWidget::processBuffer()
{
    if (m_pending.isEmpty()) return;

    int idx;
    while (!m_pending.isEmpty()) {
        if (m_isThinking) {
            idx = m_pending.indexOf("</think>");
            if (idx < 0) {
                ensureThinkSegment();
                m_thinkContent->setText(m_thinkContent->text() + m_pending);
                m_pending.clear();
                break;
            }
            ensureThinkSegment();
            if (idx > 0)
                m_thinkContent->setText(m_thinkContent->text() + m_pending.left(idx));
            m_everHadContent = true;
            m_isThinking = false;
            m_pending = m_pending.mid(idx + 8);
            startTextSegment();
        } else {
            idx = m_pending.indexOf("<think>");
            if (idx < 0) {
                ensureTextSegment();
                m_textLabel->setText(m_textLabel->text() + m_pending);
                m_pending.clear();
                break;
            }
            ensureTextSegment();
            if (idx > 0)
                m_textLabel->setText(m_textLabel->text() + m_pending.left(idx));
            m_everHadContent = true;
            m_isThinking = true;
            m_pending = m_pending.mid(idx + 7);
            startThinkSegment();
        }
    }
}

void MessageWidget::setThinking(bool thinking)
{
    if (m_isThinking == thinking) return;

    processBuffer();

    m_textLabel = nullptr;
    m_thinkContainer = nullptr;
    m_thinkToggle = nullptr;
    m_thinkScroll = nullptr;
    m_thinkContent = nullptr;
    m_thinkAnim = nullptr;

    m_isThinking = thinking;

    if (thinking)
        startThinkSegment();
    else
        startTextSegment();

    emit sizeChanged();
}

void MessageWidget::finish()
{
    processBuffer();
    m_textLabel = nullptr;
    m_thinkContainer = nullptr;
    m_thinkToggle = nullptr;
    m_thinkScroll = nullptr;
    m_thinkContent = nullptr;
    m_thinkAnim = nullptr;
    emit sizeChanged();
}

void MessageWidget::setContent(const QString &text)
{
    if (m_everHadContent) return;
    m_pending = text;
    processBuffer();
    emit sizeChanged();
}

void MessageWidget::ensureTextSegment()
{
    if (!m_textLabel)
        startTextSegment();
}

void MessageWidget::ensureThinkSegment()
{
    if (!m_thinkContent)
        startThinkSegment();
}

void MessageWidget::startTextSegment()
{
    auto *label = new QLabel(this);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setContentsMargins(0, 0, 0, 0);
    m_layout->addWidget(label);
    m_textLabel = label;
}

void MessageWidget::startThinkSegment()
{
    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *toggle = new QToolButton(container);
    toggle->setText(QStringLiteral("\u25B6 Show thinking"));
    toggle->setCheckable(true);
    toggle->setChecked(false);
    toggle->setStyleSheet(
        "QToolButton { border: none; color: #4a9eff; font-weight: bold; }"
        "QToolButton:hover { color: #6ab4ff; }"
    );
    toggle->setCursor(Qt::PointingHandCursor);
    toggle->setToolButtonStyle(Qt::ToolButtonTextOnly);

    auto *scroll = new QScrollArea(container);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setVisible(false);
    scroll->setMaximumHeight(QWIDGETSIZE_MAX);

    auto *content = new QLabel();
    content->setWordWrap(true);
    content->setTextInteractionFlags(Qt::TextSelectableByMouse);
    content->setContentsMargins(20, 2, 0, 2);
    scroll->setWidget(content);

    connect(toggle, &QToolButton::toggled, this, [this, toggle, scroll](bool checked) {
        scroll->setVisible(checked);
        toggle->setText(checked ? QStringLiteral("\u25BC Hide thinking") : QStringLiteral("\u25B6 Show thinking"));
        emit sizeChanged();
    });

    layout->addWidget(toggle);
    layout->addWidget(scroll);
    m_layout->addWidget(container);

    m_thinkContainer = container;
    m_thinkToggle = toggle;
    m_thinkScroll = scroll;
    m_thinkContent = content;
    m_thinkAnim = anim;
}
