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
    if (m_isThinking) {
        if (!m_thinkContent)
            startThinkSegment();
        m_thinkContent->setText(m_thinkContent->text() + token);
        if (m_thinkScroll->maximumHeight() > 0)
            recalculateThinkHeight();
    } else {
        if (!m_textLabel)
            startTextSegment();
        m_textLabel->setText(m_textLabel->text() + token);
    }
}

void MessageWidget::setThinking(bool thinking)
{
    if (m_isThinking == thinking)
        return;

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
    if (m_thinkScroll && m_thinkScroll->maximumHeight() > 0)
        recalculateThinkHeight();
    m_textLabel = nullptr;
    m_thinkContainer = nullptr;
    m_thinkToggle = nullptr;
    m_thinkScroll = nullptr;
    m_thinkContent = nullptr;
    m_thinkAnim = nullptr;
    emit sizeChanged();
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
    scroll->setMaximumHeight(0);

    auto *content = new QLabel();
    content->setWordWrap(true);
    content->setTextInteractionFlags(Qt::TextSelectableByMouse);
    content->setContentsMargins(20, 2, 0, 2);
    scroll->setWidget(content);

    auto *anim = new QPropertyAnimation(scroll, "maximumHeight", this);
    anim->setDuration(250);
    anim->setEasingCurve(QEasingCurve::InOutQuad);

    connect(toggle, &QToolButton::toggled, this, [this, toggle, scroll, content, anim](bool checked) {
        anim->stop();
        if (checked) {
            int prev = scroll->maximumHeight();
            scroll->setMaximumHeight(QWIDGETSIZE_MAX);
            scroll->adjustSize();
            int target = qMin(scroll->minimumSizeHint().height(), 600);
            scroll->setMaximumHeight(prev);
            anim->setStartValue(0);
            anim->setEndValue(qMax(target, 50));
        } else {
            anim->setStartValue(scroll->maximumHeight());
            anim->setEndValue(0);
        }
        toggle->setText(checked ? QStringLiteral("\u25BC Hide thinking") : QStringLiteral("\u25B6 Show thinking"));
        anim->start();
    });

    connect(anim, &QPropertyAnimation::valueChanged, this, [this]() {
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

void MessageWidget::recalculateThinkHeight()
{
    if (!m_thinkScroll || !m_thinkContent)
        return;
    int prev = m_thinkScroll->maximumHeight();
    m_thinkScroll->setMaximumHeight(QWIDGETSIZE_MAX);
    m_thinkScroll->adjustSize();
    int target = qMin(m_thinkScroll->minimumSizeHint().height(), 600);
    target = qMax(target, 50);
    if (target > prev) {
        m_thinkScroll->setMaximumHeight(target);
        emit sizeChanged();
    } else {
        m_thinkScroll->setMaximumHeight(prev);
    }
}
