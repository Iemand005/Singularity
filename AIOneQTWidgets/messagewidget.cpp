#include "messagewidget.h"
#include "ui_messagewidget.h"

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
            idx = m_pending");
            if (idx < 0) {
                ui->thinkContent->setText(ui->thinkContent->text() + m_pending);
                m_pending.clear();
                break;
            }
            if (idx > 0)
                ui->thinkContent->setText(ui->thinkContent->text() + m_pending.left(idx));
            m_everHadContent = true;
            m_isThinking = false;
            m_pending = m_pending.mid(idx + 8);
        } else {
            idx = m_pending.indexOf(" 생각은 ");
            if (idx < 0) {
                ui->textLabel->setText(ui->textLabel->text() + m_pending);
                m_pending.clear();
                break;
            }
            if (idx > 0)
                ui->textLabel->setText(ui->textLabel->text() + m_pending.left(idx));
            m_everHadContent = true;
            m_isThinking = true;
            m_pending = m_pending.mid(idx + 7);
        }
    }
}

void MessageWidget::setThinking(bool thinking)
{
    if (m_isThinking == thinking) return;

    processBuffer();
    m_isThinking = thinking;
    emit sizeChanged();
}

void MessageWidget::finish()
{
    processBuffer();
    emit sizeChanged();
}

void MessageWidget::setContent(const QString &text)
{
    if (m_everHadContent || m_hasStreamedContent) return;
    m_pending = text;
    processBuffer();
    emit sizeChanged();
}