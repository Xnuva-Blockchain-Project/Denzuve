// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <qt/splashscreen.h>

#include <clientversion.h>
#include <common/system.h>
#include <interfaces/handler.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>
#include <qt/guiutil.h>
#include <qt/networkstyle.h>
#include <qt/walletmodel.h>
#include <util/translation.h>

#include <functional>

#include <QApplication>
#include <QCloseEvent>
#include <QPainter>
#include <QFontMetrics>
#include <QRadialGradient>
#include <QScreen>


// Common XNUVA Blockchain Project splash standard.
// Coin-specific artwork; dynamic version and startup status.
SplashScreen::SplashScreen(const NetworkStyle* networkStyle)
    : QWidget()
{
    const qreal dpr = qApp->devicePixelRatio();

    const QSize logicalSize(960, 540);

    const QSize pixelSize(
        qRound(logicalSize.width() * dpr),
        qRound(logicalSize.height() * dpr));

    pixmap = QPixmap(pixelSize);
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(QColor(5, 13, 26));

    const QString titleText = CLIENT_NAME;

    const QString versionText =
        QString::fromStdString(FormatFullVersion());

    const QString networkText =
        networkStyle->getTitleAddText();

    QPainter painter(&pixmap);

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Coordinates are logical pixels, not device pixels.
    const QRect canvas(QPoint(0, 0), logicalSize);

    const QPixmap artwork(":/icons/denzuve_splash");

    if (!artwork.isNull()) {
        const QPixmap scaled = artwork.scaled(
            pixelSize,
            Qt::KeepAspectRatioByExpanding,
            Qt::SmoothTransformation);

        const QRect crop(
            (scaled.width() - pixelSize.width()) / 2,
            (scaled.height() - pixelSize.height()) / 2,
            pixelSize.width(),
            pixelSize.height());

        painter.drawPixmap(canvas, scaled, crop);
    } else {
        // Readable fallback if artwork is unavailable.
        QFont titleFont = QApplication::font();
        titleFont.setPixelSize(54);
        titleFont.setBold(true);

        painter.setFont(titleFont);
        painter.setPen(QColor(220, 231, 245));

        painter.drawText(
            QRect(40, 120, 880, 100),
            Qt::AlignCenter,
            titleText);

        QFont projectFont = QApplication::font();
        projectFont.setPixelSize(20);

        painter.setFont(projectFont);

        painter.drawText(
            QRect(40, 230, 880, 50),
            Qt::AlignCenter,
            QStringLiteral("XNUVA Blockchain Project"));
    }

    // Bottom-left panel, consistent across the wallet family.
    const QRect statusPanel(18, 418, 410, 116);

    painter.fillRect(
        statusPanel,
        QColor(5, 13, 26, 245));

    QFont versionFont = QApplication::font();
    versionFont.setPixelSize(14);
    versionFont.setBold(true);

    painter.setFont(versionFont);
    painter.setPen(Qt::white);

    painter.drawText(
        QRect(30, 424, 386, 23),
        Qt::AlignLeft | Qt::AlignVCenter,
        QStringLiteral("Version ") + versionText);

    QFont networkFont = QApplication::font();
    networkFont.setPixelSize(13);

    painter.setFont(networkFont);
    painter.setPen(QColor(185, 205, 225));

    const QString displayedNetwork =
        networkText.isEmpty()
            ? QStringLiteral("Mainnet")
            : networkText;

    painter.drawText(
        QRect(30, 449, 386, 20),
        Qt::AlignLeft | Qt::AlignVCenter,
        displayedNetwork);

    painter.end();

    setWindowTitle(titleText + " " + networkText);

    setFixedSize(logicalSize);

    if (QScreen* screen = QGuiApplication::primaryScreen()) {
        move(screen->availableGeometry().center()
             - rect().center());
    }

    installEventFilter(this);
    GUIUtil::handleCloseWindowShortcut(this);
}

SplashScreen::~SplashScreen()
{
    if (m_node) unsubscribeFromCoreSignals();
}

void SplashScreen::setNode(interfaces::Node& node)
{
    assert(!m_node);
    m_node = &node;
    subscribeToCoreSignals();
    if (m_shutdown) m_node->startShutdown();
}

void SplashScreen::shutdown()
{
    m_shutdown = true;
    if (m_node) m_node->startShutdown();
}

bool SplashScreen::eventFilter(QObject * obj, QEvent * ev) {
    if (ev->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
        if (keyEvent->key() == Qt::Key_Q) {
            shutdown();
        }
    }
    return QObject::eventFilter(obj, ev);
}

static void InitMessage(SplashScreen *splash, const std::string &message)
{
    bool invoked = QMetaObject::invokeMethod(splash, "showMessage",
        Qt::QueuedConnection,
        Q_ARG(QString, QString::fromStdString(message)),
        Q_ARG(int, Qt::AlignBottom|Qt::AlignHCenter),
        Q_ARG(QColor, QColor(55,55,55)));
    assert(invoked);
}

static void ShowProgress(SplashScreen *splash, const std::string &title, int nProgress, bool resume_possible)
{
    InitMessage(splash, title + std::string("\n") +
            (resume_possible ? SplashScreen::tr("(press q to shutdown and continue later)").toStdString()
                                : SplashScreen::tr("press q to shutdown").toStdString()) +
            strprintf("\n%d", nProgress) + "%");
}

void SplashScreen::subscribeToCoreSignals()
{
    // Connect signals to client
    m_handler_init_message = m_node->handleInitMessage([this](const std::string& message) {
        InitMessage(this, message);
    });
    m_handler_show_progress = m_node->handleShowProgress([this](const std::string& title, int nProgress, bool resume_possible) {
        ShowProgress(this, title, nProgress, resume_possible);
    });
    m_handler_init_wallet = m_node->handleInitWallet([this]() { handleLoadWallet(); });
}

void SplashScreen::handleLoadWallet()
{
#ifdef ENABLE_WALLET
    if (!WalletModel::isWalletEnabled()) return;
    m_handler_load_wallet = m_node->walletLoader().handleLoadWallet([this](std::unique_ptr<interfaces::Wallet> wallet) {
        m_connected_wallet_handlers.emplace_back(wallet->handleShowProgress([this](const std::string& title, int nProgress) {
            ShowProgress(this, title, nProgress, /*resume_possible=*/false);
        }));
        m_connected_wallets.emplace_back(std::move(wallet));
    });
#endif
}

void SplashScreen::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    m_handler_init_message->disconnect();
    m_handler_show_progress->disconnect();
    for (const auto& handler : m_connected_wallet_handlers) {
        handler->disconnect();
    }
    m_connected_wallet_handlers.clear();
    m_connected_wallets.clear();
}

void SplashScreen::showMessage(const QString &message, int alignment, const QColor &color)
{
    curMessage = message;
    curAlignment = alignment;
    curColor = color;
    update();
}

void SplashScreen::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.drawPixmap(0, 0, pixmap);

    QFont statusFont = font();
    statusFont.setPixelSize(13);

    painter.setFont(statusFont);
    painter.setPen(QColor(235, 245, 255));

    painter.drawText(
        QRect(30, 472, 386, 56),
        Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
        curMessage);
}

void SplashScreen::closeEvent(QCloseEvent *event)
{
    shutdown(); // allows an "emergency" shutdown during startup
    event->ignore();
}
