#include "messagewidget.h"

MessageWidget::MessageWidget(QWidget *parent)
    : QWidget(parent)
    , m_layout(new QVBoxLayout(this))
{
    m_layout->setContentsMargins(0, 2, 0, 2);
    m_layout->setSpacing(2);

    m_versionBar = new QWidget(this);
    auto *hb = new QHBoxLayout(m_versionBar);
    hb->setContentsMargins(0, 2, 0, 2);
    hb->setSpacing(4);

    m_prevBtn = new QPushButton(QStringLiteral("\u25C0"), m_versionBar);
    m_prevBtn->setFixedWidth(24);
    m_prevBtn->setToolTip("Previous version");
    m_prevBtn->setEnabled(false);

    m_versionLabel = new QLabel("1/1", m_versionBar);
    m_versionLabel->setAlignment(Qt::AlignCenter);

    m_nextBtn = new QPushButton(QStringLiteral("\u25B6"), m_versionBar);
    m_nextBtn->setFixedWidth(24);
    m_nextBtn->setToolTip("Next version");
    m_nextBtn->setEnabled(false);

    m_regenerateBtn = new QPushButton("Regenerate", m_versionBar);
    m_regenerateBtn->setToolTip("Generate a new response with the same context");

    hb->addStretch();
    hb->addWidget(m_prevBtn);
    hb->addWidget(m_versionLabel);
    hb->addWidget(m_nextBtn);
    hb->addWidget(m_regenerateBtn);
    hb->addStretch();

    m_versionBar->setVisible(false);
    m_layout->addWidget(m_versionBar);

    connect(m_prevBtn, &QPushButton::clicked, this, &MessageWidget::prevRequested);
    connect(m_nextBtn, &QPushButton::clicked, this, &MessageWidget::nextRequested);
    connect(m_regenerateBtn, &QPushButton::clicked, this, &MessageWidget::regenerateRequested);
}

void MessageWidget::setVersionInfo(size_t current, size_t total)
{
    if (total < 1) {
        m_versionBar->setVisible(false);
        return;
    }
    m_versionBar->setVisible(true);
    bool multi = total > 1;
    m_prevBtn->setVisible(multi);
    m_nextBtn->setVisible(multi);
    m_versionLabel->setVisible(multi);
    m_versionLabel->setText(QString("%1/%2").arg(current + 1).arg(total));
    m_prevBtn->setEnabled(current > 0);
    m_nextBtn->setEnabled(current + 1 < total);
    m_regenerateBtn->setEnabled(true);
}

void MessageWidget::appendToken(const QString &token)
{
    m_hasStreamedContent = true;
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
    emit sizeChanged();
}

void MessageWidget::setContent(const QString &text)
{
    if (m_everHadContent || m_hasStreamedContent) return;
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

    auto *content = new QLabel();
    content->setWordWrap(true);
    content->setTextInteractionFlags(Qt::TextSelectableByMouse);
    content->setContentsMargins(20, 2, 0, 2);
    scroll->setWidget(content);

    auto *anim = new QPropertyAnimation(container, "maximumHeight", this);
    anim->setDuration(200);
    anim->setEasingCurve(QEasingCurve::InOutQuad);

    int collapsedH = toggle->sizeHint().height() + layout->contentsMargins().top() + layout->contentsMargins().bottom();
    if (collapsedH < 10) collapsedH = 24;

    scroll->setVisible(false);
    container->setMaximumHeight(collapsedH);

    connect(anim, &QPropertyAnimation::valueChanged, this, [this]() {
        emit sizeChanged();
    });

    connect(toggle, &QToolButton::toggled, this, [this, toggle, scroll, container, anim, collapsedH](bool checked) {
        anim->stop();
        if (checked) {
            scroll->setVisible(true);
            container->setMaximumHeight(QWIDGETSIZE_MAX);
            int target = container->minimumSizeHint().height();
            container->setMaximumHeight(collapsedH);
            anim->setStartValue(collapsedH);
            anim->setEndValue(target);
        } else {
            anim->setStartValue(container->maximumHeight());
            anim->setEndValue(collapsedH);
        }
        toggle->setText(checked ? QStringLiteral("\u25BC Hide thinking") : QStringLiteral("\u25B6 Show thinking"));
        anim->start();
    });

    connect(anim, &QPropertyAnimation::finished, this, [this, toggle, scroll, container]() {
        if (toggle->isChecked()) {
            container->setMaximumHeight(QWIDGETSIZE_MAX);
        } else {
            scroll->setVisible(false);
        }
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
    m_thinkCollapsedHeight = collapsedH;
}
